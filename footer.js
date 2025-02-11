function _mallocString(str) {
	// https://github.com/kripken/emscripten/blob/3ebf0eed375120626ae5c2233b26bf236ea90046/src/preamble.js#L148
	// at most 4 bytes per UTF-8 code point, +1 for the trailing '\0'
	var len = (str.length << 2) + 1;
	var ptr = _malloc(len);
	stringToUTF8(str, ptr, len);
	return ptr;
}

function _markdown(renderer, text, options) {
	if (typeof text !== 'string') text = '';
	var str = _mallocString(text);
	var size = lengthBytesUTF8(text); // excludes null terminator

	if (typeof options !== 'object' || options === null) options = {};
	var nofollow = options['nofollow'] ? 1 : 0;
	var target = typeof options['target'] === 'string' ? _mallocString(options['target']) : 0;
	var toc_id_prefix = typeof options['tocIdPrefix'] === 'string' ? _mallocString(options['tocIdPrefix']) : 0;
	var enable_toc = options['enableToc'] ? 1 : 0;

	var ptr = renderer(str, size, nofollow, target, toc_id_prefix, enable_toc);
	var string = UTF8ToString(ptr);

	_free(ptr);
	_free(toc_id_prefix);
	_free(target);
	_free(str);

	return string;
}

function markdown(text, options) {
	return _markdown(_default_renderer, text, options);
}

function markdownWiki(text,	options) {
	return _markdown(_wiki_renderer, text, options);
}

window['markdown'] = markdown;
window['markdownWiki'] = markdownWiki;
})();
