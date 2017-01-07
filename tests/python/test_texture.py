from renderlib.texture import Texture
from renderlib.image import Image
import pytest

@pytest.mark.parametrize('filename,tex_type', [
    ('tests/data/star.png', Texture.TextureType.texture_rectangle),
    ('tests/data/zombie.jpg', Texture.TextureType.texture_2d),
])
def test_texture_from_image(context, filename, tex_type):
    img = Image.from_file(filename)
    assert img

    tex = Texture.from_image(img, tex_type)
    assert tex