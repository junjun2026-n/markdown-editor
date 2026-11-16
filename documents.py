"""Document decoding and atomic writes, independent of the UI."""
import codecs
import os
from pathlib import Path
import tempfile


def read_document(path):
    raw = Path(path).read_bytes()
    if raw.startswith(codecs.BOM_UTF8):
        encoding = 'utf-8-sig'
    elif raw.startswith((codecs.BOM_UTF16_LE, codecs.BOM_UTF16_BE)):
        encoding = 'utf-16'
    else:
        encoding = 'utf-8'
        try:
            raw.decode(encoding)
        except UnicodeDecodeError:
            encoding = 'gb18030'
    text = raw.decode(encoding)
    if '\x00' in text:
        raise ValueError('文件包含二进制内容，请选择 Markdown 或文本文件。')
    newline = '\r\n' if '\r\n' in text else ('\r' if '\r' in text else '\n')
    return text.replace('\r\n', '\n').replace('\r', '\n'), encoding, newline


def write_document(path, text, encoding='utf-8', newline='\n'):
    """Replace only after a complete write; keep the original on failure."""
    target = Path(path).absolute()
    data = text.replace('\n', newline).encode(encoding)
    descriptor, temporary = tempfile.mkstemp(prefix='.' + target.name + '.', suffix='.tmp', dir=target.parent)
    try:
        with os.fdopen(descriptor, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
        os.replace(temporary, target)
    finally:
        if os.path.exists(temporary):
            os.unlink(temporary)
