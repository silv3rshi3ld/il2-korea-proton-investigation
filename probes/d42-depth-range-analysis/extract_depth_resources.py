import collections
import hashlib
import math
import os
import struct
import sys
import traceback

import renderdoc as rd


output = open(os.environ["IL2_RD_OUTPUT"], "w", encoding="utf-8")
sys.stdout = output
sys.stderr = output
structured_buffers = []
dump_dir = os.environ.get("IL2_RD_DUMP_DIR")


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


def resources(node):
    return {
        str(item.AsResourceId())
        for item in walk(node)
        if item.type.basetype == rd.SDBasic.Resource
    }


def decode_depth(value):
    mantissa = value & 0xfff
    exponent = (value >> 12) & 0xf
    if exponent & 8:
        exponent -= 16
    return mantissa / 4095.0 * math.pow(10.0, exponent)


def summarize_depth_u32x2(image_id, raw, width, height):
    expected = width * height * 8
    usable = min(len(raw), expected)
    usable -= usable % 8
    texels = list(struct.iter_unpack("<2I", raw[:usable]))
    print("depth-u32x2 id=%s texels=%d expected_bytes=%d actual_bytes=%d" %
          (image_id, len(texels), expected, len(raw)))
    for component in range(2):
        values = [texel[component] for texel in texels]
        counts = collections.Counter(values)
        print("component=%d min=%d max=%d zero=%d unique=%d common=%s" % (
            component,
            min(values) if values else 0,
            max(values) if values else 0,
            counts.get(0, 0),
            len(counts),
            repr(counts.most_common(16)),
        ))
    decoded = []
    invalid = 0
    for x, mask in texels:
        near = decode_depth(x & 0xffff)
        far = decode_depth(x >> 16)
        if near > far:
            invalid += 1
        decoded.append((near, far, mask))
    near_values = [entry[0] for entry in decoded]
    far_values = [entry[1] for entry in decoded]
    spans = [entry[1] - entry[0] for entry in decoded]
    print("decoded invalid_near_gt_far=%d near=[%.9g,%.9g] far=[%.9g,%.9g] span=[%.9g,%.9g]" % (
        invalid,
        min(near_values) if near_values else 0.0,
        max(near_values) if near_values else 0.0,
        min(far_values) if far_values else 0.0,
        max(far_values) if far_values else 0.0,
        min(spans) if spans else 0.0,
        max(spans) if spans else 0.0,
    ))
    for y in range(height):
        row = decoded[y * width:(y + 1) * width]
        if not row:
            break
        empty = sum(1 for near, far, mask in row if near == 0.0 and far == 0.0 and mask == 0)
        full_mask = sum(1 for _, _, mask in row if mask == 0x7fffffff)
        print("row=%02d empty=%d full_mask=%d near_min=%.6g near_max=%.6g far_min=%.6g far_max=%.6g" % (
            y,
            empty,
            full_mask,
            min(entry[0] for entry in row),
            max(entry[0] for entry in row),
            min(entry[1] for entry in row),
            max(entry[1] for entry in row),
        ))
    print("first_decoded=%s" % repr(decoded[:64]))


def summarize_float_image(image_id, raw, width, height, components):
    expected = width * height * components * 4
    usable = min(len(raw), expected)
    usable -= usable % (components * 4)
    texels = list(struct.iter_unpack("<" + "f" * components, raw[:usable]))
    print("float-image id=%s components=%d texels=%d expected_bytes=%d actual_bytes=%d" %
          (image_id, components, len(texels), expected, len(raw)))
    for component in range(components):
        values = [texel[component] for texel in texels]
        finite = [value for value in values if math.isfinite(value)]
        print("float-component=%d finite=%d nan=%d inf=%d min=%.9g max=%.9g zero=%d" % (
            component,
            len(finite),
            sum(math.isnan(value) for value in values),
            sum(math.isinf(value) for value in values),
            min(finite) if finite else 0.0,
            max(finite) if finite else 0.0,
            sum(value == 0.0 for value in values),
        ))
    print("first_float_texels=%s" % repr(texels[:64]))


try:
    capture_path = os.environ["IL2_RDC_PATH"]
    rd.InitialiseReplay(rd.GlobalEnvironment(), [])
    capture = rd.OpenCaptureFile()
    result = capture.OpenFile(capture_path, "", None)
    if result != rd.ResultCode.Succeeded:
        raise RuntimeError("OpenFile failed: " + str(result))
    structured = capture.GetStructuredData()
    structured_buffers.extend(structured.buffers)
    chunks = structured.chunks
    print("capture=%s chunks=%d buffers=%d" %
          (capture_path, len(chunks), len(structured.buffers)))

    names = {}
    for index, chunk in enumerate(chunks):
        if str(chunk.name) != "vkSetDebugUtilsObjectNameEXT":
            continue
        object_id = resource(chunk, "Object")
        object_name = child(chunk, "ObjectName")
        if object_id is not None and object_name is not None:
            names[object_id] = str(object_name.AsString())

    target_ids = {"ResourceId::26830", "ResourceId::26842", "ResourceId::32578"}
    images = {}
    for index, chunk in enumerate(chunks):
        if str(chunk.name) != "vkCreateImage":
            continue
        create_info = child(chunk, "CreateInfo")
        extent = None if create_info is None else child(create_info, "extent")
        if extent is None:
            continue
        image_id = resource(chunk, "Image")
        dimensions = (
            integer(extent, "width"),
            integer(extent, "height"),
            integer(extent, "depth"),
        )
        image = {
            "chunk": index,
            "name": names.get(image_id),
            "dimensions": dimensions,
            "format": integer(create_info, "format"),
            "samples": integer(create_info, "samples"),
            "mips": integer(create_info, "mipLevels"),
            "layers": integer(create_info, "arrayLayers"),
        }
        lowered = (image["name"] or "").lower()
        if image_id in target_ids or "depth" in lowered or dimensions == (80, 34, 1):
            images[image_id] = image
            print("image id=%s metadata=%s" % (image_id, repr(image)))

    views = {}
    for index, chunk in enumerate(chunks):
        if str(chunk.name) != "vkCreateImageView":
            continue
        create_info = child(chunk, "CreateInfo")
        image_id = None if create_info is None else resource(create_info, "image")
        if image_id not in images:
            continue
        view_id = resource(chunk, "View")
        views[view_id] = image_id
        print("view chunk=%d id=%s image=%s name=%r format=%s type=%s" % (
            index,
            view_id,
            image_id,
            names.get(view_id),
            integer(create_info, "format"),
            integer(create_info, "viewType"),
        ))

    for index, chunk in enumerate(chunks):
        if str(chunk.name) != "vkGetDescriptorEXT":
            continue
        matched = resources(chunk) & set(views)
        if matched:
            print("descriptor chunk=%d views=%s images=%s" % (
                index,
                repr(sorted(matched)),
                repr(sorted({views[view] for view in matched})),
            ))

    for index, chunk in enumerate(chunks):
        if str(chunk.name) != "Internal::Initial Contents":
            continue
        image_id = resource(chunk, "id")
        image = images.get(image_id)
        if image is None:
            continue
        contents = child(chunk, "Contents")
        if contents is None:
            print("initial id=%s without contents" % image_id)
            continue
        raw = bytes(structured.buffers[contents.AsInt()])
        print("initial chunk=%d id=%s bytes=%d sha256=%s" % (
            index, image_id, len(raw), hashlib.sha256(raw).hexdigest()))
        width, height, depth = image["dimensions"]
        if dump_dir is not None and image["format"] == 101 and depth == 1:
            os.makedirs(dump_dir, exist_ok=True)
            safe_id = image_id.replace(":", "_")
            dump_path = os.path.join(dump_dir, "depth-range-%s.bin" % safe_id)
            with open(dump_path, "wb") as dump:
                dump.write(raw)
            print("depth-dump id=%s path=%s" % (image_id, dump_path))
        if image["format"] == 101 and depth == 1:
            summarize_depth_u32x2(image_id, raw, width, height)
        elif image["format"] == 107 and depth == 1:
            print("unexpected-u32x4-depth-candidate id=%s bytes=%d" %
                  (image_id, len(raw)))
        elif image["format"] == 103 and depth == 1:
            summarize_float_image(image_id, raw, width, height, 2)
        elif image["format"] == 100 and depth == 1:
            summarize_float_image(image_id, raw, width, height, 1)
        elif image["format"] == 109 and depth == 1:
            summarize_float_image(image_id, raw, width, height, 4)

    capture.Shutdown()
    rd.ShutdownReplay()
except BaseException:
    traceback.print_exc()
finally:
    output.flush()
    output.close()

os._exit(0)
