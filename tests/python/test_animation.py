from renderlib.animation import AnimationInstance
from renderlib.mesh import Mesh

def test_animation_play(context):
    mesh = Mesh.from_file('tests/data/zombie.mesh')
    assert len(mesh.animations) > 0

    inst = AnimationInstance(mesh.animations[0])
    inst.play(1.234)