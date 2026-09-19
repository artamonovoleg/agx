#define VERTEX_HEAP_OFFSET 0x4700

#include "window.h"
#include <agx/agx.h>
#include <agx/agx_swapchain.h>
#include <agx/agx_driver_programs.h>
#include <agx/agx_shader.h>

#define RENDER_W 512
#define RENDER_H 512

typedef struct
{
  float position[4];
  float color[4];
} vertex;

static const vertex k_triangle[] = {
  {{0.0f, 0.8f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 1.0f}},
  {{-0.8f, -0.8f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f, 1.0f}},
  {{0.8f, -0.8f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f, 1.0f}},
};

#define DYNAMIC_STATE_OFFSET 0x100
#define DYNAMIC_STATE_BYTES  0x100

#define VERTEX_COUNT ((uint32_t)(sizeof(k_triangle) / sizeof(k_triangle[0])))

static void
fragment_store_masked(Agx_Shader_Builder* builder)
{
  agx_shader_store_surface(builder, 4, AGX_SHADER_CHANNEL_WIDTH_8, 0);
}

int
main(void)
{
  Agx_Device device;
  if (!agx_create_device(&device))
  {
    fprintf(stderr, "no AGX device\n");
    return 1;
  }

  agx_usc_profile_init(device);

  Agx_Command_Queue_Desc queue_desc;
  agx_command_queue_desc_init(&queue_desc);
  Agx_Command_Queue queue = agx_create_command_queue(device, &queue_desc);
  if (queue.id == 0)
  {
    fprintf(stderr, "demo_hello_triangle: %s\n", agx_refusal());
    return 1;
  }

  Agx_Bo shader_pool;
  if (!agx_shader_pool_create(device, 0x10000, &shader_pool))
  {
    fprintf(stderr, "the shader pool could not be allocated\n");
    return 1;
  }

  uint8_t                 fragment_program[512];
  uint8_t                 vertex_program[256];
  Agx_Shader_Instruction  shader_storage[48];
  Agx_Shader_Builder      shader_builder;
  Agx_Shader_Instruction* instruction;
  size_t                  fragment_program_size;
  size_t                  vertex_program_size;

  agx_shader_builder_init(&shader_builder, shader_storage, 48);
  agx_shader_varying_read(&shader_builder, 0, 1, AGX_SHADER_VARYING_FORM_INTERPOLATED);
  agx_shader_varying_read(&shader_builder, 1, 2, AGX_SHADER_VARYING_FORM_INTERPOLATED);
  agx_shader_varying_read(&shader_builder, 2, 3, AGX_SHADER_VARYING_FORM_INTERPOLATED);

  agx_shader_move_immediate(&shader_builder, 3, 0x3f800000u);
  agx_shader_convert_to_surface(
    &shader_builder, 0, true, AGX_SHADER_CONVERT_FORM_PAIR, 0, 1, AGX_SHADER_NUMERIC_FORMAT_UNORM8
  );
  agx_shader_convert_to_surface(
    &shader_builder, 1, false, AGX_SHADER_CONVERT_FORM_PAIR, 2, 3, AGX_SHADER_NUMERIC_FORMAT_UNORM8
  );
  {
    Agx_Shader_Instruction* w = agx_shader_wait_for_store(&shader_builder, true);

    if (w)
    {
      w->raw_bits = AGX_SHADER_WAIT_OPERAND_SECOND;
    }
  }
  agx_shader_store_surface(&shader_builder, 4, AGX_SHADER_CHANNEL_WIDTH_8, 0);
  {
    Agx_Shader_Instruction* w = agx_shader_wait_for_store(&shader_builder, false);

    if (w)
    {
      w->raw_bits = AGX_SHADER_WAIT_OPERAND_THIRD;
    }
  }
  agx_shader_stop(&shader_builder);
  fragment_program_size = (size_t)agx_shader_encode(&shader_builder, fragment_program, (uint32_t)sizeof(fragment_program));

  const bool fragment_writes_coverage_mask = agx_shader_writes_coverage_mask(&shader_builder);

  agx_shader_builder_init(&shader_builder, shader_storage, 48);
  agx_shader_get_special(&shader_builder, 0, AGX_SHADER_SPECIAL_VERTEX_INDEX)->long_form = false;
  agx_shader_scale_index(
    &shader_builder, 4, 0, (uint32_t)sizeof(vertex), AGX_SHADER_INDEX_FORM_FULL, AGX_SHADER_INDEX_SOURCE_FORM_DISCARD, true, false
  );

  agx_shader_load(&shader_builder, 0, 0, 4, 4, 0, 5, AGX_SHADER_LOAD_FORM_ORDINARY, 1);
  instruction = agx_shader_load(&shader_builder, 4, 0, 4, 3, 16, 0, AGX_SHADER_LOAD_FORM_ORDINARY, 1);
  instruction->base_discard = true;
  agx_shader_output_move(&shader_builder, 0, 0, true);
  agx_shader_output_move(&shader_builder, 1, 1, false);
  agx_shader_output_move(&shader_builder, 2, 2, false);
  agx_shader_output_move(&shader_builder, 3, 3, false);
  agx_shader_output_move(&shader_builder, 4, 4, false)->long_form = false;
  agx_shader_output_move(&shader_builder, 5, 5, false);
  agx_shader_output_move(&shader_builder, 6, 6, false);
  agx_shader_vertex_export(&shader_builder, 0, 0, 2);
  agx_shader_vertex_export(&shader_builder, 1, 1, 4);
  agx_shader_vertex_export(&shader_builder, 2, 2, 8);
  agx_shader_vertex_export(&shader_builder, 3, 3, 0);
  agx_shader_vertex_export(&shader_builder, 4, 4, 1);
  agx_shader_vertex_export(&shader_builder, 5, 5, 0);
  agx_shader_vertex_export(&shader_builder, 6, 6, 0);
  agx_shader_stop(&shader_builder);
  vertex_program_size = (size_t)agx_shader_encode(&shader_builder, vertex_program, (uint32_t)sizeof(vertex_program));

  if (fragment_program_size == 0 || vertex_program_size == 0)
  {
    fprintf(stderr, "the shader builder produced nothing\n");
    return 1;
  }

  uint8_t           varying_linkage[64] = {0};
  Agx_Varying_Group k_groups[1] = {{0}};
  uint32_t          varying_linkage_size = 0;

  agx_varying_group_init(&k_groups[0], 3);
  varying_linkage_size = agx_varying_linkage_build(k_groups, 1, varying_linkage, (uint32_t)sizeof(varying_linkage));

  if (varying_linkage_size == 0)
  {
    fprintf(stderr, "the varying linkage table did not build\n");
    return 1;
  }

  const size_t k_vertex_shader_offset = 0x3c0;
  const size_t k_varying_linkage_offset = (k_vertex_shader_offset + fragment_program_size + 0x7f) & ~(size_t)0x7f;
  const size_t k_fragment_shader_offset = (k_varying_linkage_offset + varying_linkage_size + 0x7f) & ~(size_t)0x7f;

  Agx_Pool_Layout pool_layout;
  agx_pool_layout_init(&pool_layout, &shader_pool, (uint32_t)(k_fragment_shader_offset + vertex_program_size));

  const size_t k_driver_vertex_shader_offset = 0x3c0;
  const size_t k_driver_varying_linkage_offset = 0x480;
  const size_t k_driver_fragment_shader_offset = 0x540;

  if (shader_pool.cpu)
  {
    uint8_t* pool = (uint8_t*)shader_pool.cpu;

    if (!agx_pool_append_driver_programs(&pool_layout) || !agx_pool_append_end_of_tile_program(&pool_layout))
    {
      fprintf(stderr, "demo_hello_triangle: the driver's programs did not fit the pool\n");
      return 1;
    }

    fprintf(
      stderr,
      "demo_hello_triangle: pool layout: vertex stage entry 0x%05x, state loader 0x%05x\n",
      pool_layout.vertex_stage_entry,
      pool_layout.state_loader
    );
    memset(pool + k_driver_vertex_shader_offset, 0, fragment_program_size);
    memset(pool + k_driver_varying_linkage_offset, 0, varying_linkage_size);
    memset(pool + k_driver_fragment_shader_offset, 0, vertex_program_size);

    memcpy(pool + k_vertex_shader_offset, fragment_program, fragment_program_size);
    memcpy(pool + k_varying_linkage_offset, varying_linkage, varying_linkage_size);
    memcpy(pool + k_fragment_shader_offset, vertex_program, vertex_program_size);
  }

  Agx_Bo_Desc descriptors_desc;
  agx_bo_desc_init(&descriptors_desc);
  agx_bo_desc_set_size(&descriptors_desc, 0x20000);
  Agx_Bo descriptors = agx_bo_alloc(device, &descriptors_desc);

  const uint32_t varying_components = 3;

  Agx_Bo_Desc pipeline_low_desc;
  agx_bo_desc_init(&pipeline_low_desc);
  agx_bo_desc_set_space(&pipeline_low_desc, AGX_BO_SPACE_USC);
  agx_bo_desc_set_size(&pipeline_low_desc, 0x10000);
  Agx_Bo pipeline_low = agx_bo_alloc(device, &pipeline_low_desc);

  agx_driver_state_write(&pipeline_low, varying_components);

  Window window;
  if (!window_open(&window, RENDER_W, RENDER_H, "IOKit Triangle"))
  {
    fprintf(stderr, "could not open a window\n");
    return 1;
  }

  Agx_Bo background_object_program;
  if (!agx_background_object_program_create(device, &background_object_program))
  {
    fprintf(stderr, "the background object program could not be allocated\n");
    return 1;
  }

  Agx_Bo_Desc vertex_heap_desc;
  agx_bo_desc_init(&vertex_heap_desc);
  agx_bo_desc_set_suballocation(&vertex_heap_desc, &descriptors, VERTEX_HEAP_OFFSET);
  Agx_Bo vertex_heap = agx_bo_alloc(device, &vertex_heap_desc);

  Agx_Bo_Desc encoder_desc;
  agx_bo_desc_init(&encoder_desc);
  agx_bo_desc_set_space(&encoder_desc, AGX_BO_SPACE_USC);
  agx_bo_desc_set_size(&encoder_desc, 0x8000);
  Agx_Bo encoder = agx_bo_alloc(device, &encoder_desc);

  Agx_Bo_Desc pool_program_arguments_desc;
  agx_bo_desc_init(&pool_program_arguments_desc);
  agx_bo_desc_set_size(&pool_program_arguments_desc, 0x8000);
  Agx_Bo pool_program_arguments = agx_bo_alloc(device, &pool_program_arguments_desc);

  if (pool_program_arguments.cpu)
  {

    agx_pool_arguments_set_geometry(pool_program_arguments.cpu, vertex_heap.gpu_va);
  }

  float clear_color[4] = {0.05f, 0.05f, 0.08f, 1.0f};

  Agx_Bo_Desc driver_program_arguments_desc;
  agx_bo_desc_init(&driver_program_arguments_desc);
  agx_bo_desc_set_size(&driver_program_arguments_desc, 0x8000);
  Agx_Bo driver_program_arguments = agx_bo_alloc(device, &driver_program_arguments_desc);

  agx_driver_arguments_init(&driver_program_arguments);

  Agx_Bo_Desc pipeline_programs_desc;
  agx_bo_desc_init(&pipeline_programs_desc);
  agx_bo_desc_set_space(&pipeline_programs_desc, AGX_BO_SPACE_CMDBUF);
  agx_bo_desc_set_size(&pipeline_programs_desc, 0x8000);
  Agx_Bo pipeline_programs = agx_bo_alloc(device, &pipeline_programs_desc);

  Agx_Bo_Desc argument_table_desc;
  agx_bo_desc_init(&argument_table_desc);
  agx_bo_desc_set_space(&argument_table_desc, AGX_BO_SPACE_CMDBUF);
  agx_bo_desc_set_size(&argument_table_desc, 0x8000);
  Agx_Bo argument_table = agx_bo_alloc(device, &argument_table_desc);

  if (argument_table.cpu)
  {
  }

  Agx_Bo_Desc pipeline_state_desc;
  agx_bo_desc_init(&pipeline_state_desc);
  agx_bo_desc_set_space(&pipeline_state_desc, AGX_BO_SPACE_USC);
  agx_bo_desc_set_size(&pipeline_state_desc, 0x8000);
  Agx_Bo pipeline_state = agx_bo_alloc(device, &pipeline_state_desc);

  Agx_Pipeline_Desc pipeline;
  agx_pipeline_desc_init(&pipeline);
  agx_pipeline_desc_set_varying_components(&pipeline, varying_components);

  agx_pipeline_desc_set_varying_groups(&pipeline, 1);

  agx_pipeline_desc_set_shader_code(&pipeline, &shader_pool, (uint32_t)k_varying_linkage_offset);

  agx_pipeline_desc_set_pass_type(&pipeline, AGX_PASS_TYPE_OPAQUE);
  if (pipeline_state.cpu)
  {
    agx_pipeline_state_write(&pipeline, pipeline_state.cpu);
  }

  Agx_Bo_Desc copy_operands_desc;
  agx_bo_desc_init(&copy_operands_desc);
  agx_bo_desc_set_space(&copy_operands_desc, AGX_BO_SPACE_CMDBUF);
  agx_bo_desc_set_size(&copy_operands_desc, 0x9480);
  Agx_Bo copy_operands = agx_bo_alloc(device, &copy_operands_desc);

  if (pipeline_programs.cpu)
  {
    uint32_t vertex_caller_bytes = agx_pipeline_vertex_call_program_place(
      (uint8_t*)pipeline_programs.cpu,
      (uint32_t)pipeline_programs.size,
      AGX_CALLER_SHAPE_MINIMAL,
      copy_operands.gpu_va + AGX_BUFFER_COPY_OPERAND_SLOT,
      agx_pool_arguments_vertex_uniforms(pool_program_arguments.gpu_va),
      agx_driver_arguments_vertex_table(driver_program_arguments.gpu_va),
      shader_pool.gpu_va,
      pool_layout.vertex_stage_entry,
      pool_layout.state_loader
    );
    if (vertex_caller_bytes == 0)
    {
      fprintf(stderr, "demo_hello_triangle: the pipeline's vertex caller did not place\n");
      exit(1);
    }
    if (agx_pipeline_fragment_call_program_place(
          (uint8_t*)pipeline_programs.cpu,
          AGX_PIPELINE_PROGRAMS_FRAGMENT_CALLER_OFFSET,
          (uint32_t)pipeline_programs.size,
          vertex_caller_bytes,
          AGX_CALLER_SHAPE_MINIMAL,
          agx_pool_arguments_fragment_block(pool_program_arguments.gpu_va),
          agx_driver_arguments_fragment_table(driver_program_arguments.gpu_va),
          shader_pool.gpu_va,
          (uint32_t)k_fragment_shader_offset,
          varying_components
        ) == 0)
    {
      fprintf(stderr, "demo_hello_triangle: the pipeline's fragment caller did not place\n");
      exit(1);
    }
  }

  Agx_Tile_Programs tile_programs;
  if (!agx_tile_programs_create(
        device,
        &driver_program_arguments,
        &background_object_program,
        &shader_pool,
        pool_layout.end_of_tile_second_program + AGX_END_OF_TILE_ENTRY_CLEAR,
        &driver_program_arguments,
        &tile_programs
      ))
  {
    fprintf(stderr, "no tile programs\n");
    return 1;
  }

  agx_tile_programs_write_register_release(&tile_programs);
  if (agx_tile_programs_build_vertex_shader_caller(
        &tile_programs,
        agx_driver_arguments_vertex_shader_block(driver_program_arguments.gpu_va),
        (uint32_t)k_vertex_shader_offset,
        fragment_writes_coverage_mask,
        0u
      ) == 0)
  {
    fprintf(stderr, "demo_hello_triangle: the tile heap's vertex-shader caller did not build\n");
    exit(1);
  }
  agx_tile_programs_set_vertex_shader(&tile_programs, (uint32_t)k_vertex_shader_offset);

  Agx_Swapchain  swapchain;
  const uint32_t width = window.width;
  const uint32_t height = window.height;

  if (!agx_swapchain_create(&swapchain, window.layer, window.width, window.height, 2, AGX_SWAPCHAIN_FORMAT_BGRA8_UNORM))
  {
    fprintf(stderr, "could not create a swapchain\n");
    return 1;
  }

  Agx_Bo render_target_buffer[2];
  for (uint32_t i = 0; i < swapchain.image_count; ++i)
  {
    render_target_buffer[i] = agx_alloc_iosurface(device, IOSurfaceGetID(swapchain.images[i]), width, height);
  }

  Agx_Bo render_target = render_target_buffer[0];

  Agx_Bo_Desc resource_group_req = {
    .request_kind = AGX_BO_REQUEST_KIND_RESOURCE_GROUP,
    .sub_cpu = 0x80,
  };
  Agx_Bo resource_group_object = agx_bo_alloc(device, &resource_group_req);

  Agx_Bo segment_list = agx_alloc_shmem(device, 0x4000, false);
  Agx_Bo cmd_memory = agx_alloc_shmem(device, 0x4000, true);

  const struct
  {
    Agx_Bo*  program;
    size_t   offset;
    Agx_Bo*  target;
    uint32_t shift;
    uint64_t delta;
  } instruction_immediates[] = {

    {&tile_programs.heap, 0x646, &argument_table, 13, 0},

    {&pipeline_state, 0x015, &tile_programs.heap, 14, 0},
  };

  for (uint32_t i = 0; i < sizeof(instruction_immediates) / sizeof(instruction_immediates[0]); ++i)
  {
    uint8_t* at = (uint8_t*)instruction_immediates[i].program->cpu + instruction_immediates[i].offset;

    uint64_t address = instruction_immediates[i].target->gpu_va + instruction_immediates[i].delta;
    uint16_t v = (uint16_t)(address >> instruction_immediates[i].shift);
    memcpy(at, &v, sizeof(v));
  }

  uint32_t resources[] = {
    (uint32_t)vertex_heap.index,
    (uint32_t)render_target_buffer[0].index,
    (uint32_t)render_target_buffer[1].index,
  };

  uint64_t resource_group = resource_group_object.index;

  agx_resource_group_update(device, resource_group, resources, (uint32_t)(sizeof(resources) / sizeof(resources[0])));
  agx_queue_set_resource_groups(queue, resource_group);

  fprintf(stderr, "%zu resource(s) resident\n", sizeof(resources) / sizeof(resources[0]));

  vertex* vertices = (vertex*)((uint8_t*)descriptors.cpu + VERTEX_HEAP_OFFSET);

  Agx_Fence fence;
  if (!agx_create_fence(device, &fence))
  {
    fprintf(stderr, "could not create a fence\n");
    return 1;
  }
  uint64_t fence_value = 0;

  uint64_t frame = 0;

  while (window_poll(&window))
  {
    uint32_t image_index = 0;
    agx_swapchain_acquire(&swapchain, &image_index);

    render_target = render_target_buffer[image_index];

    Agx_Cmd cmd;
    agx_cmd_begin(&cmd, &cmd_memory, &encoder, &segment_list);
    agx_cmd_set_driver_arguments(&cmd, &driver_program_arguments);
    agx_cmd_set_dynamic_state(&cmd, &pipeline_state, DYNAMIC_STATE_OFFSET, DYNAMIC_STATE_BYTES);

    Agx_Render_Pass_Desc pass;
    agx_render_pass_desc_init(&pass);
    pass.attachment_count = 1;
    pass.width = width;
    pass.height = height;
    agx_render_pass_desc_set_tile_programs(&pass, tile_programs);
    pass.tile_program_heap = argument_table.gpu_va;
    pass.uniform_slice = descriptors.gpu_va;
    memcpy(pass.clear_color, clear_color, sizeof(clear_color));

    if (!agx_cmd_begin_render_pass(&cmd, &pass))
    {
      fprintf(stderr, "no room in the command buffer for the render encoder\n");
      break;
    }

    agx_cmd_set_render_target(&cmd, 0, &render_target, width, height, AGX_IMAGE_CHANNELS_R8G8B8A8);

    agx_cmd_bind_vertex_pipeline(&cmd, AGX_VERTEX_PIPELINE_DRIVER, pipeline_programs.gpu_va, varying_components);

    agx_cmd_push_driver_state(&cmd, &pipeline_low);
    agx_cmd_set_pipeline_state(&cmd, &pipeline_state, 0);

    agx_cmd_set_viewport(&cmd, 0, (Agx_Viewport_Desc) {0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f});
    agx_cmd_set_scissor_rect(&cmd, 0, (Agx_Scissor_Desc) {0, 0, width, height});

    agx_cmd_set_cull_mode(&cmd, AGX_CULL_MODE_NONE);

    agx_cmd_set_pass_type(&cmd, AGX_PASS_TYPE_OPAQUE);

    agx_cmd_draw(&cmd, AGX_PRIMITIVE_TRIANGLES, VERTEX_COUNT, 1, 0);

    agx_cmd_end_render_pass(&cmd);

    agx_cmd_end(&cmd);

    Agx_Segment_List_Builder residency;
    agx_cmd_residency_begin(&cmd, &residency);

    agx_segment_list_record(&residency, cmd.record_offset[0], cmd.record_size[0]);
    agx_segment_list_add(&residency, &encoder, AGX_RESIDENCY_CLASS_DRIVER_A);
    agx_segment_list_add(&residency, &pool_program_arguments, AGX_RESIDENCY_CLASS_DRIVER_A);
    agx_segment_list_add(&residency, &driver_program_arguments, AGX_RESIDENCY_CLASS_DRIVER_B);
    agx_segment_list_add(&residency, &pipeline_programs, AGX_RESIDENCY_CLASS_DRIVER_A);
    agx_segment_list_add(&residency, &tile_programs.heap, AGX_RESIDENCY_CLASS_DRIVER_B);
    agx_segment_list_add(&residency, &argument_table, AGX_RESIDENCY_CLASS_DRIVER_B);
    agx_segment_list_add(&residency, &pipeline_state, AGX_RESIDENCY_CLASS_SHADER_HEAP);
    agx_segment_list_add(&residency, &shader_pool, AGX_RESIDENCY_CLASS_APP);
    agx_segment_list_add(&residency, &background_object_program, AGX_RESIDENCY_CLASS_APP);

    agx_segment_list_add(&residency, &render_target, AGX_RESIDENCY_CLASS_RENDER_TARGET);
    agx_segment_list_add(&residency, &pipeline_low, AGX_RESIDENCY_CLASS_APP);

    if (agx_segment_list_finish(&residency) == 0)
    {
      fprintf(stderr, "residency lists did not fit\n");
      break;
    }

    Agx_Image color;
    agx_image_init(&color);
    agx_image_set_image(&color, render_target.gpu_va, width, height);
    agx_image_set_layout(&color, AGX_IMAGE_LAYOUT_INTERCHANGE);
    agx_image_set_extended(&color, true);
    agx_image_set_compression(&color, render_target.gpu_va + AGX_DRAWABLE_METADATA_BYTES(width, height));

    agx_image_set_grid(&color, AGX_IMAGE_GRID_TEXEL);
    agx_image_set_swizzle(&color, AGX_CHANNEL_B, AGX_CHANNEL_G, AGX_CHANNEL_R, AGX_CHANNEL_A);
    agx_image_set_tiles_per_row(&color, AGX_TILES_X(width));
    agx_argument_table_set_image(&driver_program_arguments, &color);

    agx_image_set_grid(&color, AGX_IMAGE_GRID_SAMPLE);
    agx_image_set_swizzle(&color, AGX_CHANNEL_R, AGX_CHANNEL_G, AGX_CHANNEL_B, AGX_CHANNEL_A);
    agx_image_set_tiles_per_row(&color, 4 * AGX_TILES_X(width) - 3);

    agx_driver_arguments_set_render_target(&driver_program_arguments, 0, 0, &color);
    agx_driver_arguments_set_render_target(&driver_program_arguments, 1, 0, &color);

    for (uint32_t v = 0; v < VERTEX_COUNT; ++v)
    {
      vertices[v] = k_triangle[v];
    }

    if (!agx_queue_submit(queue, &cmd, 1))
    {
      fprintf(stderr, "submit failed on frame %u\n", frame);
      break;
    }
    agx_queue_signal_fence(queue, fence, ++fence_value);

    if (!agx_wait_fence(fence, fence_value, 1000))
    {
      frame++;
      continue;
    }

    agx_swapchain_present(&swapchain);

    frame++;
  }

  agx_destroy_command_queue(device, &queue);
  agx_destroy_device(device);
  agx_swapchain_destroy(&swapchain);
  window_close(&window);
  return 0;
}
