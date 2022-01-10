//
// Created by Mike Smith on 2022/1/9.
//

#include <luisa-compute.h>
#include <scene/material.h>
#include <scene/interaction.h>
#include <scene/pipeline.h>
#include <util/sampling.h>

namespace luisa::render {

using namespace luisa::compute;

struct LambertParams {
    float color[3];
    uint color_texture_id;
};

class LambertMaterial final : public Material {

private:
    LambertParams _params;

public:
    LambertMaterial(Scene *scene, const SceneNodeDesc *desc) noexcept
        : Material{scene, desc}, _params{} {
        auto color = clamp(
            desc->property_float3_or_default(
                "color", make_float3(desc->property_float_or_default("color", 1.0f))),
            0.0f, 1.0f);
        auto color_texture = desc->property_path_or_default("color", {});
        _params.color[0] = color.x;
        _params.color[1] = color.y;
        _params.color[2] = color.z;
        if (color_texture.empty()) {
            _params.color_texture_id = ~0u;
        } else {
            // TODO: load texture
            LUISA_INFO("Loading color texture for lambert material: '{}'.", color_texture.string());
        }
    }
    [[nodiscard]] string_view impl_type() const noexcept override { return "lambert"; }
    [[nodiscard]] uint property_flags() const noexcept override {
        auto flags = property_flag_reflective;
        if (_params.color[0] == 0.0f &&
            _params.color[1] == 0.0f &&
            _params.color[1] == 0.0f) {
            flags |= property_flag_black;
        }
        return flags;
    }
    [[nodiscard]] uint encode(Pipeline &pipeline, CommandBuffer &command_buffer, const Shape *) const noexcept override {
        auto [buffer_view, buffer_id] = pipeline.arena_buffer<LambertParams>(sizeof(LambertParams));
        command_buffer << buffer_view.copy_from(&_params);
        return buffer_id;
    }
    [[nodiscard]] luisa::unique_ptr<Closure> decode(const Pipeline &pipeline, const Interaction &it) const noexcept override;
};

}// namespace luisa::render

LUISA_STRUCT(luisa::render::LambertParams, color, color_texture_id){};

namespace luisa::render {

class LambertClosure final : public Material::Closure {

private:
    const Interaction &_interaction;
    Float3 _f;
    Float _cos_wo;
    Bool _front_face;

public:
    LambertClosure(const Interaction &it, Float3 color) noexcept
        : _interaction{it},
          _f{std::move(color) * inv_pi},
          _cos_wo{dot(it.wo(), it.shading().n())},
          _front_face{_cos_wo > 0.0f} {}

private:
    [[nodiscard]] Material::Evaluation evaluate(Expr<float3> wi) const noexcept override {
        auto n = _interaction.shading().n();
        auto wo = _interaction.wo();
        auto cos_wi = dot(n, wi);
        auto same_hemisphere = cos_wi * _cos_wo > 0.0f;
        auto pdf = ite(same_hemisphere & _front_face, cosine_hemisphere_pdf(abs(cos_wi)), 0.0f);
        return {.f = _f, .pdf = std::move(pdf)};
    }

    [[nodiscard]] Material::Sample sample(Sampler::Instance &sampler) const noexcept override {
        auto wi_local = sample_cosine_hemisphere(sampler.generate_2d());
        auto pdf = ite(_front_face, cosine_hemisphere_pdf(wi_local.z), 0.0f);
        wi_local.z *= sign(_cos_wo);
        return {.wi = _interaction.shading().local_to_world(wi_local),
                .eval = {.f = _f, .pdf = pdf}};
    }
};

luisa::unique_ptr<Material::Closure> LambertMaterial::decode(const Pipeline &pipeline, const Interaction &it) const noexcept {
    auto params = pipeline.buffer<LambertParams>(it.shape()->material_buffer_id()).read(0u);
    auto color = def<float3>(params.color);
    $if(params.color_texture_id != ~0u) {
        color = pipeline.tex2d(params.color_texture_id).sample(it.uv());
    };
    return luisa::make_unique<LambertClosure>(it, std::move(color));
}

}// namespace luisa::render

LUISA_RENDER_MAKE_SCENE_NODE_PLUGIN(luisa::render::LambertMaterial)