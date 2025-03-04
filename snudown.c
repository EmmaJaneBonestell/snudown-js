#include "markdown.h"
#include "html.h"
#include "autolink.h"

#include <string.h>

#define SNUDOWN_VERSION "1.7.0"

enum snudown_renderer_mode {
	RENDERER_USERTEXT = 0,
	RENDERER_WIKI,
	RENDERER_COUNT
};

struct snudown_renderopt {
	struct html_renderopt html;
	int nofollow;
	const char *target;
};

struct snudown_renderer {
	struct sd_markdown* main_renderer;
	struct sd_markdown* toc_renderer;
	struct module_state* state;
	struct module_state* toc_state;
};

struct module_state {
	struct sd_callbacks callbacks;
	struct snudown_renderopt options;
};

static int sundown_initialized[RENDERER_COUNT];
static struct snudown_renderer sundown[RENDERER_COUNT];

static char* html_element_whitelist[] = {"tr", "th", "td", "table", "tbody", "thead", "tfoot", "caption", NULL};
static char* html_attr_whitelist[] = {"colspan", "rowspan", "cellspacing", "cellpadding", "scope", NULL};

static struct module_state usertext_toc_state;
static struct module_state wiki_toc_state;
static struct module_state usertext_state;
static struct module_state wiki_state;

static const unsigned int snudown_default_md_flags =
	MKDEXT_NO_INTRA_EMPHASIS |
	MKDEXT_SUPERSCRIPT |
	MKDEXT_AUTOLINK |
	MKDEXT_STRIKETHROUGH |
	MKDEXT_TABLES;

static const unsigned int snudown_default_render_flags =
	HTML_SKIP_HTML |
	HTML_SKIP_IMAGES |
	HTML_SAFELINK |
	HTML_ESCAPE |
	HTML_USE_XHTML;

static const unsigned int snudown_wiki_render_flags =
	HTML_SKIP_HTML |
	HTML_SAFELINK |
	HTML_ALLOW_ELEMENT_WHITELIST |
	HTML_ESCAPE |
	HTML_USE_XHTML;

static void
snudown_link_attr(struct buf *ob, const struct buf *link, void *opaque)
{
	struct snudown_renderopt *options = opaque;

	if (options->nofollow)
		BUFPUTSL(ob, " rel=\"nofollow\"");

	if (options->target != NULL) {
		BUFPUTSL(ob, " target=\"");
		bufputs(ob, options->target);
		bufputc(ob, '\"');
	}
}

static struct sd_markdown* make_custom_renderer(struct module_state* state,
												const unsigned int renderflags,
												const unsigned int markdownflags,
												int toc_renderer) {
	if(toc_renderer) {
		sdhtml_toc_renderer(&state->callbacks,
			(struct html_renderopt *)&state->options);
	} else {
		sdhtml_renderer(&state->callbacks,
			(struct html_renderopt *)&state->options,
			renderflags);
	}

	state->options.html.link_attributes = &snudown_link_attr;
	state->options.html.html_element_whitelist = html_element_whitelist;
	state->options.html.html_attr_whitelist = html_attr_whitelist;

	return sd_markdown_new(
		markdownflags,
		16,
		64,
		&state->callbacks,
		&state->options
	);
}

void init_default_renderer(void) {
	if (sundown_initialized[RENDERER_USERTEXT]) return;
	sundown_initialized[RENDERER_USERTEXT] = 1;
	sundown[RENDERER_USERTEXT].main_renderer = make_custom_renderer(&usertext_state, snudown_default_render_flags, snudown_default_md_flags, 0);
	sundown[RENDERER_USERTEXT].toc_renderer = make_custom_renderer(&usertext_toc_state, snudown_default_render_flags, snudown_default_md_flags, 1);
	sundown[RENDERER_USERTEXT].state = &usertext_state;
	sundown[RENDERER_USERTEXT].toc_state = &usertext_toc_state;
}

void init_wiki_renderer(void) {
	if (sundown_initialized[RENDERER_WIKI]) return;
	sundown_initialized[RENDERER_WIKI] = 1;
	sundown[RENDERER_WIKI].main_renderer = make_custom_renderer(&wiki_state, snudown_wiki_render_flags, snudown_default_md_flags, 0);
	sundown[RENDERER_WIKI].toc_renderer = make_custom_renderer(&wiki_toc_state, snudown_wiki_render_flags, snudown_default_md_flags, 1);
	sundown[RENDERER_WIKI].state = &wiki_state;
	sundown[RENDERER_WIKI].toc_state = &wiki_toc_state;
}

/* size param is necessary because text may contain null */
const char*
snudown_md(char* text, size_t size, int nofollow, char* target, char* toc_id_prefix, int renderer, int enable_toc) {
	struct buf ib, *ob;
	char* result_text;
	struct snudown_renderer _snudown;
	unsigned int flags;

	memset(&ib, 0x0, sizeof(struct buf));

	/* set up buffer */
	ib.data = (uint8_t*) text;
	ib.size = size;

	if (renderer < 0 || renderer >= RENDERER_COUNT) {
		return NULL;
	}

	_snudown = sundown[renderer];

	struct snudown_renderopt *options = &(_snudown.state->options);
	options->nofollow = nofollow;
	options->target = target;

	/* Output buffer */
	ob = bufnew(128);

	flags = options->html.flags;

	if (enable_toc) {
		_snudown.toc_state->options.html.toc_id_prefix = toc_id_prefix;
		sd_markdown_render(ob, ib.data, ib.size, _snudown.toc_renderer);
		_snudown.toc_state->options.html.toc_id_prefix = NULL;

		options->html.flags |= HTML_TOC;
	}

	options->html.toc_id_prefix = toc_id_prefix;

	/* do the magic */
	sd_markdown_render(ob, ib.data, ib.size, _snudown.main_renderer);

	options->html.toc_id_prefix = NULL;
	options->html.flags = flags;

	/* make a null-terminated result string - the buffer isn't */
	result_text = (char*) malloc(ob->size + 1);
	result_text[ob->size] = 0;
	if (ob->data)
		memcpy(result_text, (char*) ob->data, ob->size);

	/* Cleanup */
	bufrelease(ob);

	return result_text;
}

const char* default_renderer(char* text, size_t size, int nofollow, char* target, char* toc_id_prefix, int enable_toc) {
	init_default_renderer();
	return snudown_md(text, size, nofollow, target, toc_id_prefix, RENDERER_USERTEXT, enable_toc);
}

const char* wiki_renderer(char* text, size_t size, int nofollow, char* target, char* toc_id_prefix, int enable_toc) {
	init_wiki_renderer();
	return snudown_md(text, size, nofollow, target, toc_id_prefix, RENDERER_WIKI, enable_toc);
}
