from renderlib.text import Text
from renderlib.font import Font

def test_text(context):
    font = Font.from_file('tests/data/courier.ttf', 18)
    text = Text(font)
    text.string = 'hello world'
    assert text.string == 'hello world'
    assert text.width > 0
    assert text.height > 0
