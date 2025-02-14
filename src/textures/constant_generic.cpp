//
// Created by Mike Smith on 2022/1/26.
//

#include <base/texture.h>
#include <base/pipeline.h>

namespace luisa::render {

class ConstantGeneric final : public Texture {

private:
    float4 _v;
    uint _channels;

private:
    [[nodiscard]] TextureHandle _encode(Pipeline &pipeline, CommandBuffer &command_buffer, uint handle_tag) const noexcept override {
        return TextureHandle::encode_constant(handle_tag, _v.xyz(), _v.w);
    }

public:
    ConstantGeneric(Scene *scene, const SceneNodeDesc *desc) noexcept
        : Texture{scene, desc} {
        auto v = desc->property_float_list_or_default("v");
        if (v.size() > 4u) [[unlikely]] {
            LUISA_WARNING(
                "Too many values (count = {}) for ConstValue. "
                "Additional values will be discarded. [{}]",
                v.size(), desc->source_location().string());
            v.resize(4u);
        }
        _channels = std::max(v.size(), static_cast<size_t>(1u));
        for (auto i = 0u; i < v.size(); i++) { _v[i] = v[i]; }
    }
    [[nodiscard]] bool is_black() const noexcept override { return all(_v == 0.0f); }
    [[nodiscard]] luisa::string_view impl_type() const noexcept override { return LUISA_RENDER_PLUGIN_NAME; }
    [[nodiscard]] Float4 evaluate(
        const Pipeline &pipeline, const Interaction &,
        const Var<TextureHandle> &handle, Expr<float>) const noexcept override {
        return make_float4(handle->v(), handle->alpha());
    }
    [[nodiscard]] Category category() const noexcept override { return Category::GENERIC; }
    [[nodiscard]] uint channels() const noexcept override { return _channels; }
};

}// namespace luisa::render

LUISA_RENDER_MAKE_SCENE_NODE_PLUGIN(luisa::render::ConstantGeneric)
