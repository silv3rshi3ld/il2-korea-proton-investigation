import hashlib
import os
import re
import sys
import traceback

import renderdoc as rd


output = open(os.environ["IL2_RD_OUTPUT"], "w", encoding="utf-8")
sys.stdout = output
sys.stderr = output


def children(node):
    for index in range(node.NumChildren()):
        yield node.GetChild(index)


def walk(node):
    yield node
    for item in children(node):
        yield from walk(item)


def child(node, name):
    return node.FindChild(name)


def resource(node, name):
    item = child(node, name)
    return None if item is None else str(item.AsResourceId())


def safe_name(value):
    return re.sub(r"[^A-Za-z0-9_.-]+", "_", value).strip("_")


try:
    capture_path = os.environ["IL2_RDC_PATH"]
    dump_dir = os.environ["IL2_RD_DUMP_DIR"]
    shader_filter = os.environ.get("IL2_RD_SHADER_FILTER", "")
    os.makedirs(dump_dir, exist_ok=True)

    rd.InitialiseReplay(rd.GlobalEnvironment(), [])
    capture = rd.OpenCaptureFile()
    result = capture.OpenFile(capture_path, "", None)
    if result != rd.ResultCode.Succeeded:
        raise RuntimeError("OpenFile failed: " + str(result))
    structured = capture.GetStructuredData()
    chunks = structured.chunks
    buffers = structured.buffers
    print("capture=%s chunks=%d buffers=%d" % (capture_path, len(chunks), len(buffers)))

    names = {}
    for chunk in chunks:
        if str(chunk.name) != "vkSetDebugUtilsObjectNameEXT":
            continue
        object_id = resource(chunk, "Object")
        object_name = child(chunk, "ObjectName")
        if object_id is not None and object_name is not None:
            names[object_id] = str(object_name.AsString())

    found = 0
    dumped = 0
    for index, chunk in enumerate(chunks):
        if str(chunk.name) != "vkCreateShaderModule":
            continue
        found += 1
        module_id = resource(chunk, "ShaderModule")
        debug_name = names.get(module_id, "")
        buffer_nodes = [
            item for item in walk(chunk) if item.type.basetype == rd.SDBasic.Buffer
        ]
        if len(buffer_nodes) != 1:
            print(
                "module chunk=%d id=%s name=%r buffers=%d skipped=ambiguous"
                % (index, module_id, debug_name, len(buffer_nodes))
            )
            continue
        buffer_index = buffer_nodes[0].AsInt()
        if not 0 <= buffer_index < len(buffers):
            print(
                "module chunk=%d id=%s name=%r buffer=%d skipped=out-of-range"
                % (index, module_id, debug_name, buffer_index)
            )
            continue
        raw = bytes(buffers[buffer_index])
        sha256 = hashlib.sha256(raw).hexdigest()
        print(
            "module chunk=%d id=%s name=%r size=%d sha256=%s"
            % (index, module_id, debug_name, len(raw), sha256)
        )
        if shader_filter and shader_filter.lower() not in debug_name.lower():
            continue
        label = safe_name(debug_name) or safe_name(module_id) or ("chunk-%d" % index)
        path = os.path.join(dump_dir, "%s-%s.spv" % (label, sha256[:16]))
        with open(path, "wb") as shader:
            shader.write(raw)
        dumped += 1
        print("dump path=%s" % path)

    print("summary modules=%d dumped=%d filter=%r" % (found, dumped, shader_filter))
    output.flush()
    capture.Shutdown()
    rd.ShutdownReplay()
except Exception:
    traceback.print_exc()
    output.flush()
    try:
        rd.ShutdownReplay()
    except Exception:
        pass
    raise
finally:
    output.close()
