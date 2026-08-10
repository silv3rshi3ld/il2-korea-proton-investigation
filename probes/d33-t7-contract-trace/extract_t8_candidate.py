import hashlib
import math
import os
import struct
import sys
import traceback

import renderdoc as rd


CANDIDATE_RANGE = int(os.environ.get("IL2_RD_RANGE", "240"), 0)
CANDIDATE_FORMAT = int(os.environ.get("IL2_RD_FORMAT", "107"), 0)
DECODE_BYTES = int(os.environ.get("IL2_RD_DECODE_BYTES", "1024"), 0)

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


def integer(node, name):
    item = child(node, name)
    return None if item is None else item.AsInt()


def resource(node, name):
    item = child(node, name)
    return None if item is None else str(item.AsResourceId())


def named_integers(node, name):
    return [
        item.AsInt()
        for item in walk(node)
        if str(item.name) == name
        and item.type.basetype
        in (
            rd.SDBasic.SignedInteger,
            rd.SDBasic.UnsignedInteger,
            rd.SDBasic.Enum,
            rd.SDBasic.GPUAddress,
        )
    ]


def referenced_resources(node):
    return [
        str(item.AsResourceId())
        for item in walk(node)
        if item.type.basetype == rd.SDBasic.Resource
    ]


def as_u64(value):
    return value & ((1 << 64) - 1)


def format_float(value):
    if math.isnan(value):
        return "nan"
    if math.isinf(value):
        return "+inf" if value > 0 else "-inf"
    return "%.9g" % value


try:
    capture_path = os.environ["IL2_RDC_PATH"]
    print("capture=" + capture_path)
    print(
        "candidate-range=%d candidate-format=%d decode-bytes=%d"
        % (CANDIDATE_RANGE, CANDIDATE_FORMAT, DECODE_BYTES)
    )
    rd.InitialiseReplay(rd.GlobalEnvironment(), [])
    capture = rd.OpenCaptureFile()
    result = capture.OpenFile(capture_path, "", None)
    if result != rd.ResultCode.Succeeded:
        raise RuntimeError("OpenFile failed: " + str(result))

    structured = capture.GetStructuredData()
    chunks = structured.chunks
    print("structured chunks=%d buffers=%d" % (len(chunks), len(structured.buffers)))

    debug_names = {}
    for index, chunk in enumerate(chunks):
        if str(chunk.name) != "vkSetDebugUtilsObjectNameEXT":
            continue
        object_id = resource(chunk, "Object")
        name_node = child(chunk, "ObjectName")
        if object_id is not None and name_node is not None:
            debug_names[object_id] = str(name_node.AsString())

    buffers = {}
    for index, chunk in enumerate(chunks):
        if str(chunk.name) != "vkCreateBuffer":
            continue
        create_info = child(chunk, "CreateInfo")
        buffer_id = resource(chunk, "Buffer")
        buffers[buffer_id] = {
            "chunk": index,
            "size": integer(create_info, "size"),
            "name": debug_names.get(buffer_id),
        }

    allocations = {}
    for index, chunk in enumerate(chunks):
        if str(chunk.name) != "vkAllocateMemory":
            continue
        allocation_info = child(chunk, "AllocateInfo")
        memory_id = resource(chunk, "Memory")
        addresses = named_integers(chunk, "opaqueCaptureAddress")
        if memory_id is None or not addresses:
            continue
        allocations[memory_id] = {
            "chunk": index,
            "base": as_u64(addresses[0]),
            "size": integer(allocation_info, "allocationSize"),
        }

    buffer_bindings = {}
    for index, chunk in enumerate(chunks):
        if "BindBufferMemory" not in str(chunk.name):
            continue
        for item in walk(chunk):
            buffer_node = child(item, "buffer")
            memory_node = child(item, "memory")
            offset_node = child(item, "memoryOffset")
            if buffer_node is None or memory_node is None or offset_node is None:
                continue
            buffer_id = str(buffer_node.AsResourceId())
            memory_id = str(memory_node.AsResourceId())
            buffer_bindings[memory_id] = {
                "chunk": index,
                "buffer": buffer_id,
                "offset": offset_node.AsInt(),
            }

    candidates = []
    for index, chunk in enumerate(chunks):
        if str(chunk.name) != "vkGetDescriptorEXT":
            continue
        info = child(chunk, "DescriptorInfo")
        if info is None:
            continue
        ranges = named_integers(info, "range")
        formats = named_integers(info, "format")
        addresses = named_integers(info, "address")
        if CANDIDATE_RANGE not in ranges or CANDIDATE_FORMAT not in formats or not addresses:
            continue
        candidates.append(
            {
                "chunk": index,
                "type": integer(info, "type"),
                "range": CANDIDATE_RANGE,
                "format": CANDIDATE_FORMAT,
                "address": as_u64(addresses[0]),
                "refs": referenced_resources(chunk),
            }
        )

    print("candidate-count=%d" % len(candidates))
    initial_contents = {}
    for index, chunk in enumerate(chunks):
        if str(chunk.name) != "Internal::Initial Contents":
            continue
        memory_id = resource(chunk, "id")
        contents = child(chunk, "Contents")
        if memory_id in allocations and contents is not None:
            initial_contents[memory_id] = {
                "chunk": index,
                "data": bytes(structured.buffers[contents.AsInt()]),
            }

    for candidate_index, candidate in enumerate(candidates):
        matches = []
        for memory_id, allocation in allocations.items():
            if allocation["base"] <= candidate["address"] < allocation["base"] + allocation["size"]:
                matches.append((memory_id, allocation))

        print(
            "candidate[%d] chunk=%d type=%s address=%#x range=%d format=%d "
            "float4-elements=%d allocation-matches=%d"
            % (
                candidate_index,
                candidate["chunk"],
                candidate["type"],
                candidate["address"],
                candidate["range"],
                candidate["format"],
                candidate["range"] // 16,
                len(matches),
            )
        )

        for memory_id, allocation in matches:
            allocation_offset = candidate["address"] - allocation["base"]
            binding = buffer_bindings.get(memory_id)
            buffer_info = None if binding is None else buffers.get(binding["buffer"])
            print(
                "candidate[%d] memory=%s allocation-base=%#x allocation-size=%d "
                "allocation-offset=%d buffer-binding=%s buffer-info=%s"
                % (
                    candidate_index,
                    memory_id,
                    allocation["base"],
                    allocation["size"],
                    allocation_offset,
                    repr(binding),
                    repr(buffer_info),
                )
            )

            initial = initial_contents.get(memory_id)
            if initial is None:
                print("candidate[%d] no-initial-contents" % candidate_index)
                continue

            raw_memory = initial["data"]
            region = raw_memory[allocation_offset : allocation_offset + DECODE_BYTES]
            print(
                "candidate[%d] initial-chunk=%d captured-allocation-bytes=%d "
                "decoded-bytes=%d sha256=%s"
                % (
                    candidate_index,
                    initial["chunk"],
                    len(raw_memory),
                    len(region),
                    hashlib.sha256(region).hexdigest(),
                )
            )

            usable = len(region) - len(region) % 16
            float_values = struct.unpack("<%df" % (usable // 4), region[:usable])
            uint_values = struct.unpack("<%dI" % (usable // 4), region[:usable])
            for texel in range(usable // 16):
                float_vector = float_values[texel * 4 : texel * 4 + 4]
                uint_vector = uint_values[texel * 4 : texel * 4 + 4]
                record = texel // 8
                field = texel % 8
                declared = texel < CANDIDATE_RANGE // 16
                print(
                    "texel=%d record=%d field=%d declared=%s finite=%s "
                    "uint_values=(%s) float_reinterpretation=(%s)"
                    % (
                        texel,
                        record,
                        field,
                        "yes" if declared else "no",
                        "yes" if all(math.isfinite(value) for value in float_vector) else "no",
                        ", ".join(str(value) for value in uint_vector),
                        ", ".join(format_float(value) for value in float_vector),
                    )
                )

    capture.Shutdown()
    rd.ShutdownReplay()
except BaseException:
    traceback.print_exc()
finally:
    output.flush()
    output.close()

os._exit(0)
