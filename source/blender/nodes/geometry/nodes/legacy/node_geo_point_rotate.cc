/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "BLI_math_rotation.h"

#include "UI_interface.h"
#include "UI_resources.h"

#include "node_geometry_util.hh"

namespace blender::nodes {

static void geo_node_point_rotate_declare(NodeDeclarationBuilder &b)
{
  b.add_input<decl::Geometry>(N_("Geometry"));
  b.add_input<decl::String>(N_("Axis"));
  b.add_input<decl::Vector>(N_("Axis"), "Axis_001")
      .default_value({0.0, 0.0, 1.0})
      .subtype(PROP_XYZ);
  b.add_input<decl::String>(N_("Angle"));
  b.add_input<decl::Float>(N_("Angle"), "Angle_001").subtype(PROP_ANGLE);
  b.add_input<decl::String>(N_("Rotation"));
  b.add_input<decl::Vector>(N_("Rotation"), "Rotation_001").subtype(PROP_EULER);
  b.add_output<decl::Geometry>(N_("Geometry"));
}

static void geo_node_point_rotate_layout(uiLayout *layout, bContext *UNUSED(C), PointerRNA *ptr)
{
  NodeGeometryRotatePoints *storage = (NodeGeometryRotatePoints *)((bNode *)ptr->data)->storage;

  uiItemR(layout, ptr, "type", UI_ITEM_R_EXPAND, nullptr, ICON_NONE);
  uiItemR(layout, ptr, "space", UI_ITEM_R_EXPAND, nullptr, ICON_NONE);

  uiLayoutSetPropSep(layout, true);
  uiLayoutSetPropDecorate(layout, false);

  uiLayout *col = uiLayoutColumn(layout, false);
  if (storage->type == GEO_NODE_POINT_ROTATE_TYPE_AXIS_ANGLE) {
    uiItemR(col, ptr, "input_type_axis", 0, IFACE_("Axis"), ICON_NONE);
    uiItemR(col, ptr, "input_type_angle", 0, IFACE_("Angle"), ICON_NONE);
  }
  else {
    uiItemR(col, ptr, "input_type_rotation", 0, IFACE_("Rotation"), ICON_NONE);
  }
}

static void geo_node_point_rotate_init(bNodeTree *UNUSED(ntree), bNode *node)
{
  NodeGeometryRotatePoints *node_storage = (NodeGeometryRotatePoints *)MEM_callocN(
      sizeof(NodeGeometryRotatePoints), __func__);

  node_storage->type = GEO_NODE_POINT_ROTATE_TYPE_EULER;
  node_storage->space = GEO_NODE_POINT_ROTATE_SPACE_OBJECT;
  node_storage->input_type_axis = GEO_NODE_ATTRIBUTE_INPUT_VECTOR;
  node_storage->input_type_angle = GEO_NODE_ATTRIBUTE_INPUT_FLOAT;
  node_storage->input_type_rotation = GEO_NODE_ATTRIBUTE_INPUT_VECTOR;

  node->storage = node_storage;
}

static void geo_node_point_rotate_update(bNodeTree *ntree, bNode *node)
{
  NodeGeometryRotatePoints *node_storage = (NodeGeometryRotatePoints *)node->storage;
  update_attribute_input_socket_availabilities(
      *ntree,
      *node,
      "Axis",
      (GeometryNodeAttributeInputMode)node_storage->input_type_axis,
      node_storage->type == GEO_NODE_POINT_ROTATE_TYPE_AXIS_ANGLE);
  update_attribute_input_socket_availabilities(
      *ntree,
      *node,
      "Angle",
      (GeometryNodeAttributeInputMode)node_storage->input_type_angle,
      node_storage->type == GEO_NODE_POINT_ROTATE_TYPE_AXIS_ANGLE);
  update_attribute_input_socket_availabilities(
      *ntree,
      *node,
      "Rotation",
      (GeometryNodeAttributeInputMode)node_storage->input_type_rotation,
      node_storage->type == GEO_NODE_POINT_ROTATE_TYPE_EULER);
}

static void point_rotate__axis_angle__object_space(const int domain_size,
                                                   const VArray<float3> &axis,
                                                   const VArray<float> &angles,
                                                   MutableSpan<float3> rotations)
{
  for (const int i : IndexRange(domain_size)) {
    float old_rotation[3][3];
    eul_to_mat3(old_rotation, rotations[i]);
    float rotation[3][3];
    axis_angle_to_mat3(rotation, axis[i], angles[i]);
    float new_rotation[3][3];
    mul_m3_m3m3(new_rotation, rotation, old_rotation);
    mat3_to_eul(rotations[i], new_rotation);
  }
}

static void point_rotate__axis_angle__point_space(const int domain_size,
                                                  const VArray<float3> &axis,
                                                  const VArray<float> &angles,
                                                  MutableSpan<float3> rotations)
{
  for (const int i : IndexRange(domain_size)) {
    float old_rotation[3][3];
    eul_to_mat3(old_rotation, rotations[i]);
    float rotation[3][3];
    axis_angle_to_mat3(rotation, axis[i], angles[i]);
    float new_rotation[3][3];
    mul_m3_m3m3(new_rotation, old_rotation, rotation);
    mat3_to_eul(rotations[i], new_rotation);
  }
}

static void point_rotate__euler__object_space(const int domain_size,
                                              const VArray<float3> &eulers,
                                              MutableSpan<float3> rotations)
{
  for (const int i : IndexRange(domain_size)) {
    float old_rotation[3][3];
    eul_to_mat3(old_rotation, rotations[i]);
    float rotation[3][3];
    eul_to_mat3(rotation, eulers[i]);
    float new_rotation[3][3];
    mul_m3_m3m3(new_rotation, rotation, old_rotation);
    mat3_to_eul(rotations[i], new_rotation);
  }
}

static void point_rotate__euler__point_space(const int domain_size,
                                             const VArray<float3> &eulers,
                                             MutableSpan<float3> rotations)
{
  for (const int i : IndexRange(domain_size)) {
    float old_rotation[3][3];
    eul_to_mat3(old_rotation, rotations[i]);
    float rotation[3][3];
    eul_to_mat3(rotation, eulers[i]);
    float new_rotation[3][3];
    mul_m3_m3m3(new_rotation, old_rotation, rotation);
    mat3_to_eul(rotations[i], new_rotation);
  }
}

static void point_rotate_on_component(GeometryComponent &component,
                                      const GeoNodeExecParams &params)
{
  const bNode &node = params.node();
  const NodeGeometryRotatePoints &storage = *(const NodeGeometryRotatePoints *)node.storage;

  OutputAttribute_Typed<float3> rotation_attribute =
      component.attribute_try_get_for_output<float3>("rotation", ATTR_DOMAIN_POINT, {0, 0, 0});
  if (!rotation_attribute) {
    return;
  }

  MutableSpan<float3> rotations = rotation_attribute.as_span();
  const int domain_size = rotations.size();

  if (storage.type == GEO_NODE_POINT_ROTATE_TYPE_AXIS_ANGLE) {
    VArray<float3> axis = params.get_input_attribute<float3>(
        "Axis", component, ATTR_DOMAIN_POINT, {0, 0, 1});
    VArray<float> angles = params.get_input_attribute<float>(
        "Angle", component, ATTR_DOMAIN_POINT, 0);

    if (storage.space == GEO_NODE_POINT_ROTATE_SPACE_OBJECT) {
      point_rotate__axis_angle__object_space(domain_size, axis, angles, rotations);
    }
    else {
      point_rotate__axis_angle__point_space(domain_size, axis, angles, rotations);
    }
  }
  else {
    VArray<float3> eulers = params.get_input_attribute<float3>(
        "Rotation", component, ATTR_DOMAIN_POINT, {0, 0, 0});

    if (storage.space == GEO_NODE_POINT_ROTATE_SPACE_OBJECT) {
      point_rotate__euler__object_space(domain_size, eulers, rotations);
    }
    else {
      point_rotate__euler__point_space(domain_size, eulers, rotations);
    }
  }

  rotation_attribute.save();
}

static void geo_node_point_rotate_exec(GeoNodeExecParams params)
{
  GeometrySet geometry_set = params.extract_input<GeometrySet>("Geometry");

  geometry_set = geometry_set_realize_instances(geometry_set);

  if (geometry_set.has<MeshComponent>()) {
    point_rotate_on_component(geometry_set.get_component_for_write<MeshComponent>(), params);
  }
  if (geometry_set.has<PointCloudComponent>()) {
    point_rotate_on_component(geometry_set.get_component_for_write<PointCloudComponent>(), params);
  }
  if (geometry_set.has<CurveComponent>()) {
    point_rotate_on_component(geometry_set.get_component_for_write<CurveComponent>(), params);
  }

  params.set_output("Geometry", geometry_set);
}

}  // namespace blender::nodes

void register_node_type_geo_point_rotate()
{
  static bNodeType ntype;

  geo_node_type_base(&ntype, GEO_NODE_LEGACY_POINT_ROTATE, "Point Rotate", NODE_CLASS_GEOMETRY, 0);
  node_type_init(&ntype, blender::nodes::geo_node_point_rotate_init);
  node_type_update(&ntype, blender::nodes::geo_node_point_rotate_update);
  node_type_storage(
      &ntype, "NodeGeometryRotatePoints", node_free_standard_storage, node_copy_standard_storage);
  ntype.declare = blender::nodes::geo_node_point_rotate_declare;
  ntype.geometry_node_execute = blender::nodes::geo_node_point_rotate_exec;
  ntype.draw_buttons = blender::nodes::geo_node_point_rotate_layout;
  nodeRegisterType(&ntype);
}
