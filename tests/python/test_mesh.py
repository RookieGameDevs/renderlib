from renderlib.mesh import Mesh


def test_mesh_from_file(context):
    mesh = Mesh.from_file('tests/data/zombie.mesh')
    assert mesh


def test_mesh_from_buffer(context):
    with open('tests/data/zombie.mesh', 'rb') as fp:
        mesh_data = fp.read()
        mesh = Mesh.from_buffer(mesh_data)
        assert mesh