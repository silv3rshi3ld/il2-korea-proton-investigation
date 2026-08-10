# D43 conservative tiled-depth quantization

`ComputeDepthRange` point-samples the scene-depth range, quantizes each tile's
minimum and maximum into a 16-bit logarithmic representation, and emits a
31-bit logarithmic occupancy mask. Both later depth predicates that restore
the blocks in D39 and D40 originate in this one producer.

D43 keeps the original depth gate but makes its packed output conservative by
the smallest directly representable amounts:

- decrement a nonzero near mantissa by one unit;
- increment a nonzero, non-saturated far mantissa by one unit; and
- include the immediately adjacent bit on both sides of the occupancy mask.

This is a precision/rounding discriminator, not yet an upstream fix. It should
remove false negatives caused by native-D3D versus Vulkan math/sampling
rounding without admitting every geometrically intersecting light as D38 did.
