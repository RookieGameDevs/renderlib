from renderlib.core import MeshRenderProps
from renderlib.core import render_mesh
from renderlib.mesh import Mesh


def test_render_mesh(context):
    mesh = Mesh.from_file('tests/data/zombie.mesh')
    props = MeshRenderProps()
    render_mesh(mesh, props)