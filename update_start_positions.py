import json
import math
import numpy as np

COLOR_NAMES = {0: 'orange', 1: 'green', 2: 'blue'}
KIND_NAMES = {0: 'gear', 1: 'gear_slot', 2: 'circuit_connector', 3: 'conductor',
              4: 'corner_marker', 5: 'cube', 6: 'long_cube', 7: 'pin'}

PART_TYPE_MAP = {
    'cubes-bigCubeBlue': (6, 2),
    'cubes-CubeOrange': (5, 0),
    'cubes-CubeGreen': (5, 1),
}


def parse_dat(filename):
    with open(filename, 'r') as f:
        tokens = f.read().split()
    idx = 0
    # Header: <len> "serialization::archive" <version>
    idx += 3
    # Vector class tracking: class_id, tracking_level
    idx += 2
    count = int(tokens[idx]); idx += 1
    # Entity class tracking: class_id, tracking_level, version
    idx += 3

    entities = []
    for i in range(count):
        color = int(tokens[idx]); idx += 1
        kind = int(tokens[idx]); idx += 1
        if i == 0:
            # Affine3d class tracking appears only on first occurrence
            idx += 2
        rows = int(tokens[idx]); idx += 1
        cols = int(tokens[idx]); idx += 1
        mat = np.zeros((rows, cols))
        for r in range(rows):
            for c in range(cols):
                mat[r, c] = float(tokens[idx]); idx += 1
        entities.append({'color': color, 'kind': kind, 'pose': mat})
        print(f"  Entity {i+1}: {COLOR_NAMES[color]} {KIND_NAMES[kind]} "
              f"pos=({mat[0,3]:.4f}, {mat[1,3]:.4f}, {mat[2,3]:.4f})")
    return entities


def make_z_rotation(angle):
    m = np.eye(4)
    m[0, 0] = math.cos(angle); m[0, 1] = -math.sin(angle)
    m[1, 0] = math.sin(angle); m[1, 1] = math.cos(angle)
    return m


def corner_frame(corner):
    ct = corner['pose'][:3, 3]
    T1 = np.eye(4); T1[:3, 3] = ct
    T2 = np.eye(4); T2[:3, 3] = -ct
    R = make_z_rotation(-math.pi / 4)
    return T1 @ R @ T2 @ corner['pose']


def pcl_T_web(kind):
    m = np.eye(4)
    z_offsets = {0: 0.04, 3: 0.028, 4: 0.02, 5: 0.059, 6: 0.059}
    m[2, 3] = z_offsets.get(kind, 0.0)
    return m


def from_y_up_to_z_up_matrix(mat4x4):
    R = mat4x4[:3, :3]
    t = mat4x4[:3, 3]
    # eulerAngles(0,1,2) XYZ Tait-Bryan: ea[1] = asin(R[0,2]) for Y rotation
    sin_b = float(np.clip(R[0, 2], -1.0, 1.0))
    b = math.asin(sin_b)
    res = make_z_rotation(b)
    res[0, 3] = t[2]; res[1, 3] = t[0]; res[2, 3] = t[1]
    return res


def transform_to_vector_and_rotation(mat4x4):
    t = mat4x4[:3, 3].copy()
    angle = math.atan2(mat4x4[1, 0], mat4x4[0, 0])
    angle = math.fmod(angle + 2 * math.pi, 2 * math.pi)
    return t, angle


def to_world_frame_op(kind, start_pos, start_rot, corner):
    cf = corner_frame(corner)
    angle = start_rot - math.pi / 2
    rel = make_z_rotation(angle)
    rel[:3, 3] = start_pos
    rel[2, 3] -= 0.02
    return pcl_T_web(kind) @ cf @ rel


def angle_z_task(mat4x4):
    angle = math.atan2(mat4x4[1, 0], mat4x4[0, 0]) + math.pi / 2
    return math.fmod(angle + 2 * math.pi, 2 * math.pi)


def z_up_to_y_up_col_major(translation_z_up, angle_z):
    rx, ry, rz = translation_z_up
    tx, ty, tz = ry, rz, rx
    c = math.cos(angle_z)
    s = math.sin(angle_z)
    mat = np.array([
        [c,  0.0,  s,  tx],
        [0.0, 1.0, 0.0, ty],
        [-s, 0.0,  c,  tz],
        [0.0, 0.0, 0.0, 1.0],
    ])
    return mat.flatten(order='F').tolist()


def main():
    dat_file = 'studies/main/found_entities.dat'
    json_file = 'studies/main/benchmark_task/task_description.json'

    print("Parsing found_entities.dat ...")
    entities = parse_dat(dat_file)

    corner = next((e for e in entities if e['kind'] == 4), None)
    if corner is None:
        raise RuntimeError("No corner_marker found in dat file")
    print(f"\nCorner marker: pos=({corner['pose'][0,3]:.4f}, "
          f"{corner['pose'][1,3]:.4f}, {corner['pose'][2,3]:.4f})")

    with open(json_file, 'r', encoding='utf-8') as f:
        task = json.load(f)

    op_nodes = [n for n in task['nodes'] if 'part_type' in n]

    # Compute expected world position for each operation using current JSON data + detected corner
    op_info = []
    for op in op_nodes:
        part_type = op['part_type']
        if part_type not in PART_TYPE_MAP:
            continue
        kind, color = PART_TYPE_MAP[part_type]
        pose_data = op.get('start_pose_relative_to_corner')
        if pose_data is None:
            continue
        mat_web = np.array(pose_data, dtype=float).reshape(4, 4, order='F')
        if op.get('y-up', False):
            mat_robot = from_y_up_to_z_up_matrix(mat_web)
        else:
            mat_robot = mat_web
        start_pos, start_rot = transform_to_vector_and_rotation(mat_robot)
        world = to_world_frame_op(kind, start_pos, start_rot, corner)
        op_info.append({'node': op, 'kind': kind, 'color': color,
                        'world': world, 'start_pos': start_pos, 'start_rot': start_rot})

    # Match each operation to the closest detected entity of the same kind+color
    available = [e for e in entities if e['kind'] != 4]
    cf = corner_frame(corner)
    cf_inv = np.linalg.inv(cf)

    print("\nMatching operations to detected entities ...")
    updated = 0
    not_found = []
    for item in op_info:
        exp_xy = item['world'][:2, 3]
        candidates = [(i, e) for i, e in enumerate(available)
                      if e['kind'] == item['kind'] and e['color'] == item['color']]
        if not candidates:
            not_found.append(item['node']['id'])
            continue

        best_avail_idx, best_e = min(candidates,
                                     key=lambda ie: np.linalg.norm(ie[1]['pose'][:2, 3] - exp_xy))

        rel = cf_inv @ best_e['pose']
        t = rel[:3, 3]
        theta = angle_z_task(rel)

        new_start = z_up_to_y_up_col_major(t, theta)
        node = item['node']

        dist = np.linalg.norm(best_e['pose'][:2, 3] - exp_xy)
        print(f"  Op {node['id']:2d} ({node.get('part_type','?'):<22s}): "
              f"matched entity at pos=({best_e['pose'][0,3]:.4f}, {best_e['pose'][1,3]:.4f}) "
              f"dist={dist:.4f}m")
        print(f"             relative pos=({t[0]:.4f}, {t[1]:.4f}, {t[2]:.4f}) angle={math.degrees(theta):.1f} deg")

        node['start_pose_relative_to_corner'] = new_start
        available.pop(best_avail_idx)
        updated += 1

    print(f"\nUpdated {updated} operations.")
    if not_found:
        print(f"WARNING: No entity found for operations: {not_found}")

    with open(json_file, 'w', encoding='utf-8') as f:
        json.dump(task, f, indent=3)
    print(f"Wrote updated task description to {json_file}")


if __name__ == '__main__':
    main()
