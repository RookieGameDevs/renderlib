from renderlib.image import Image
import pytest

TEST_IMAGE_INFO = [
    ('tests/data/star.png', (31, 30), Image.Codec.PNG),
    ('tests/data/zombie.jpg', (2048, 2048), Image.Codec.JPEG),
]

@pytest.mark.parametrize('filename,size,codec', TEST_IMAGE_INFO)
def test_image_from_file(context, filename, size, codec):
    image = Image.from_file(filename)
    assert image.width == size[0]
    assert image.height == size[1]

@pytest.mark.parametrize('filename,size,codec', TEST_IMAGE_INFO)
def test_image_from_buffer(context, filename, size, codec):
    with open(filename, 'rb') as fp:
        image_data = fp.read()
        image = Image.from_buffer(image_data, codec)
        assert image.width == size[0]
        assert image.height == size[1]