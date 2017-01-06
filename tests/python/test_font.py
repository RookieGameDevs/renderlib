from renderlib.font import Font

def test_font_from_file(context):
    font = Font.from_file('tests/data/courier.ttf', 16)
    assert font

def test_font_from_buffer(context):
    with open('tests/data/courier.ttf', 'rb') as fp:
        font_data = fp.read()
        font = Font.from_buffer(font_data, 16)
        assert font