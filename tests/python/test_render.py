from matlib import Mat
from matlib import Vec
from renderlib.animation import AnimationInstance
from renderlib.core import Light
from renderlib.core import Material
from renderlib.core import MeshRenderProps
from renderlib.core import render_mesh
from renderlib.image import Image
from renderlib.mesh import Mesh
from renderlib.texture import Texture

def test_render_mesh(context):
    # load the mesh and create an animation instance
    mesh = Mesh.from_file('tests/data/zombie.mesh')
    anim = AnimationInstance(mesh.animations[0])

    # create a textured material
    img = Image.from_file('tests/data/zombie.jpg')
    texture = Texture.from_image(img, Texture.TextureType.texture_2d)
    material = Material()
    material.texture = texture
    material.color = Vec(0.3, 0.8, 0.4)  # unnecessary with textures, just test
    material.receive_light = True
    material.specular_intensity = 0.8
    material.specular_power = 4

    # create a light
    light_proj = Mat()  # light projection matrix
    light_proj.ortho(
        -5, 5,
        5, -5,
        0,
        10)
    light_view = Mat()  # light "orientation" matrix
    light_view.lookat(
        0, 5, 0,
        0, 0, 0,
        0, 1, 0)

    light = Light()
    light.transform = light_proj * light_view
    light.direction = Vec(0, -5, -5)
    light.direction.norm()
    light.color = Vec(0.8, 0.8, 0.8, 1.0)
    light.ambient_intensity = 0.3
    light.diffuse_intensity = 0.8

    # put everything together in mesh render properties instance
    props = MeshRenderProps()
    props.eye = Vec(5, 5, 5)
    props.model = props.view = props.projection = Mat()
    props.cast_shadows = True
    props.receive_shadows = True
    props.light = light
    props.animation = anim
    props.material = material

    # do an actual rendering
    render_mesh(mesh, props)