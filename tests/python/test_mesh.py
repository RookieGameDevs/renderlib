from renderlib.mesh import Mesh

def test_mesh_from_file(context):
    mesh = Mesh.from_file('tests/data/zombie.mesh')
    assert mesh


def test_mesh_from_buffer(context):
    with open('tests/data/zombie.mesh', 'rb') as fp:
        mesh_data = fp.read()
        mesh = Mesh.from_buffer(mesh_data)
        assert mesh

def test_mesh_new(context):
    vertices = [
        (-1, -1, 0),
        (1, -1, 0),
        (0, 1, 0),
    ]
    indices = [
        0,
        1,
        2
    ]
    normals = [
        (0, 0, 1),
        (0, 0, 1),
        (0, 0, 1),
    ]
    uvs = [
        (0, 0),
        (1, 0),
        (0.5, 1),
    ]
    joint_ids = [
        (0, 0, 0, 0),
        (0, 0, 0, 0),
        (0, 0, 0, 0),
    ]
    joint_weights = [
        (0, 0, 0, 0),
        (0, 0, 0, 0),
        (0, 0, 0, 0),
    ]
    mesh = Mesh(
        vertices,
        indices,
        normals,
        uvs,
        joint_ids,
        joint_weights)
    assert mesh.vertex_count == 3
    assert mesh.index_count == 3