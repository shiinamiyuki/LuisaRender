//
// Created by Mike Smith on 2022/1/10.
//

#include <luisa-compute.h>
#include <base/light_sampler.h>
#include <base/pipeline.h>

namespace luisa::render {

class UniformLightSampler final : public LightSampler {

public:
    luisa::unique_ptr<Instance> build(Pipeline &pipeline, CommandBuffer &command_buffer) const noexcept override;
    UniformLightSampler(Scene *scene, const SceneNodeDesc *desc) noexcept : LightSampler{scene, desc} {}
    [[nodiscard]] string_view impl_type() const noexcept override { return LUISA_RENDER_PLUGIN_NAME; }
};

class UniformLightSamplerInstance final : public LightSampler::Instance {

private:
    uint _light_buffer_id{};

public:
    UniformLightSamplerInstance(const LightSampler *sampler, Pipeline &pipeline, CommandBuffer &command_buffer) noexcept
        : LightSampler::Instance{pipeline, sampler} {
        auto [view, buffer_id] = pipeline.arena_buffer<uint>(pipeline.lights().size());
        _light_buffer_id = buffer_id;
        luisa::vector<uint> light_to_instance_id(pipeline.lights().size());
        std::transform(
            pipeline.lights().cbegin(), pipeline.lights().cend(),
            light_to_instance_id.begin(),
            [](auto light) noexcept {
                return Shape::Handle::encode_light_buffer_id_and_tag(
                    light.second.instance_id, light.second.tag);
            });
        command_buffer << view.copy_from(light_to_instance_id.data())
                       << compute::commit();// lifetime
    }
    void update(CommandBuffer &, float) noexcept override {}
    [[nodiscard]] Float pmf(const Interaction &it) const noexcept {
        return static_cast<float>(1.0 / static_cast<double>(pipeline().lights().size()));
    }
    [[nodiscard]] Float pmf(const Interaction &it, const SampledWavelengths &) const noexcept override { return pmf(it); }
    [[nodiscard]] LightSampler::Selection select(Sampler::Instance &sampler, const Interaction &it, const SampledWavelengths &) const noexcept override {
        using namespace luisa::compute;
        auto u = sampler.generate_1d();
        auto n = static_cast<uint>(pipeline().lights().size());
        auto i = clamp(cast<uint>(u * static_cast<float>(n)), 0u, n - 1u);
        auto instance_id_and_light_tag = pipeline().buffer<uint>(_light_buffer_id).read(i);
        auto instance_id = instance_id_and_light_tag >> Shape::Handle::light_buffer_id_shift;
        auto light_tag = instance_id_and_light_tag & Shape::Handle::light_tag_mask;
        return {instance_id, light_tag, pmf(it)};
    }
};

unique_ptr<LightSampler::Instance> UniformLightSampler::build(Pipeline &pipeline, CommandBuffer &command_buffer) const noexcept {
    return luisa::make_unique<UniformLightSamplerInstance>(this, pipeline, command_buffer);
}

}// namespace luisa::render

LUISA_RENDER_MAKE_SCENE_NODE_PLUGIN(luisa::render::UniformLightSampler)
