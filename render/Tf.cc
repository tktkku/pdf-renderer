#include "pdf-render.h"
#include "pdf-render-private.h"
#include "pdf-private.h"
#include "plutovg-private.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "plutovg-stb-truetype.h"

static int stbtt_InitFont_internal1(stbtt_fontinfo *info, unsigned char *data, int fontstart)
{
   stbtt_uint32 cmap, t;
   stbtt_int32 i,numTables;

   info->data = data;
   info->fontstart = fontstart;
   info->cff = stbtt__new_buf(NULL, 0);

   cmap = stbtt__find_table(data, fontstart, "cmap");       // required
   info->loca = stbtt__find_table(data, fontstart, "loca"); // required
   info->head = stbtt__find_table(data, fontstart, "head"); // required
   info->glyf = stbtt__find_table(data, fontstart, "glyf"); // required
   info->hhea = stbtt__find_table(data, fontstart, "hhea"); // required
   info->hmtx = stbtt__find_table(data, fontstart, "hmtx"); // required
   info->kern = stbtt__find_table(data, fontstart, "kern"); // not required
   info->gpos = stbtt__find_table(data, fontstart, "GPOS"); // not required

   if (/*!cmap || */!info->head || !info->hhea || !info->hmtx)
      return 0;
   if (info->glyf) {
      // required for truetype
      if (!info->loca) return 0;
   } else {
      // initialization for CFF / Type2 fonts (OTF)
      stbtt__buf b, topdict, topdictidx;
      stbtt_uint32 cstype = 2, charstrings = 0, fdarrayoff = 0, fdselectoff = 0;
      stbtt_uint32 cff;

      cff = stbtt__find_table(data, fontstart, "CFF ");
      if (!cff) return 0;

      info->fontdicts = stbtt__new_buf(NULL, 0);
      info->fdselect = stbtt__new_buf(NULL, 0);

      // @TODO this should use size from table (not 512MB)
      info->cff = stbtt__new_buf(data+cff, 512*1024*1024);
      b = info->cff;

      // read the header
      stbtt__buf_skip(&b, 2);
      stbtt__buf_seek(&b, stbtt__buf_get8(&b)); // hdrsize

      // @TODO the name INDEX could list multiple fonts,
      // but we just use the first one.
      stbtt__cff_get_index(&b);  // name INDEX
      topdictidx = stbtt__cff_get_index(&b);
      topdict = stbtt__cff_index_get(topdictidx, 0);
      stbtt__cff_get_index(&b);  // string INDEX
      info->gsubrs = stbtt__cff_get_index(&b);

      stbtt__dict_get_ints(&topdict, 17, 1, &charstrings);
      stbtt__dict_get_ints(&topdict, 0x100 | 6, 1, &cstype);
      stbtt__dict_get_ints(&topdict, 0x100 | 36, 1, &fdarrayoff);
      stbtt__dict_get_ints(&topdict, 0x100 | 37, 1, &fdselectoff);
      info->subrs = stbtt__get_subrs(b, topdict);

      // we only support Type 2 charstrings
      if (cstype != 2) return 0;
      if (charstrings == 0) return 0;

      if (fdarrayoff) {
         // looks like a CID font
         if (!fdselectoff) return 0;
         stbtt__buf_seek(&b, fdarrayoff);
         info->fontdicts = stbtt__cff_get_index(&b);
         info->fdselect = stbtt__buf_range(&b, fdselectoff, b.size-fdselectoff);
      }

      stbtt__buf_seek(&b, charstrings);
      info->charstrings = stbtt__cff_get_index(&b);
   }

   t = stbtt__find_table(data, fontstart, "maxp");
   if (t)
      info->numGlyphs = ttUSHORT(data+t+4);
   else
      info->numGlyphs = 0xffff;

   info->svg = -1;

   // find a cmap encoding table we understand *now* to avoid searching
   // later. (todo: could make this installable)
   // the same regardless of glyph.
   if (cmap)
   {
        numTables = ttUSHORT(data + cmap + 2);
        info->index_map = 0;
        for (i=0; i < numTables; ++i) {
            stbtt_uint32 encoding_record = cmap + 4 + 8 * i;
            // find an encoding we understand:
            switch(ttUSHORT(data+encoding_record)) {
                case STBTT_PLATFORM_ID_MICROSOFT:
                    switch (ttUSHORT(data+encoding_record+2)) {
                    case STBTT_MS_EID_UNICODE_BMP:
                    case STBTT_MS_EID_UNICODE_FULL:
                        // MS/Unicode
                        info->index_map = cmap + ttULONG(data+encoding_record+4);
                        break;
                    }
                    break;
                case STBTT_PLATFORM_ID_UNICODE:
                    // Mac/iOS has these
                    // all the encodingIDs are unicode, so we don't bother to check it
                    info->index_map = cmap + ttULONG(data+encoding_record+4);
                    break;
            }
        }
        if (info->index_map == 0)
            return 0;
    }
   info->indexToLocFormat = ttUSHORT(data+info->head + 50);
   return 1;
}

plutovg_font_face_t* plutovg_font_face_load_from_data1(const void* data, 
    unsigned int length, int ttcindex, 
    plutovg_destroy_func_t destroy_func, void* closure)
{
    stbtt_fontinfo info;
    int offset = stbtt_GetFontOffsetForIndex((unsigned char*)data, ttcindex);
    if (offset == -1 || !stbtt_InitFont_internal1(&info, (unsigned char*)data, offset)) {
        if (destroy_func)
            destroy_func(closure);
        return NULL;
    }
    plutovg_font_face_t* face = (plutovg_font_face_t*)malloc(sizeof(plutovg_font_face_t));
    face->ref_count = 1;
    face->info = info;
    stbtt_GetFontVMetrics(&face->info, &face->ascent, &face->descent, &face->line_gap);
    stbtt_GetFontBoundingBox(&face->info, &face->x1, &face->y1, &face->x2, &face->y2);
    memset(face->glyphs, 0, sizeof(face->glyphs));
    face->destroy_func = destroy_func;
    face->closure = closure;
    return face;
}
void load_font_from_external(pdf_render_t* context, pdf_font_descriptor_t* font_descriptor)
{
    //TODO
    char fontname[256] = { 0 };
    bool isSerif = (font_descriptor->flags & 0x02) != 0;
    sprintf(fontname, "fonts/Noto%sSC-", isSerif ? "Serif" : "Sans");
    //\xCB\xCE\xCC\xE5 -> SimSun
    if (font_descriptor->fontWeight <= 0)
    {
        strcat(fontname, "Regular.ttf");
    }
    else if (font_descriptor->fontWeight <= 100)
    {
        strcat(fontname, "Thin.ttf");
    }
    else if (font_descriptor->fontWeight <= 200)
    {
        strcat(fontname, "ExtraLight.ttf");
    }
    else if (font_descriptor->fontWeight <= 300)
    {
        strcat(fontname, "Light.ttf");
    }
    else if (font_descriptor->fontWeight == 400)
    {
        strcat(fontname, "Regular.ttf");
    }
    else if (font_descriptor->fontWeight <= 500)
    {
        strcat(fontname, "Medium.ttf");
    }
    else if (font_descriptor->fontWeight <= 600)
    {
        strcat(fontname, "SemiBold.ttf");
    }
    else if (font_descriptor->fontWeight <= 700)
    {
        strcat(fontname, "Bold.ttf");
    }
    else if (font_descriptor->fontWeight <= 800)
    {
        strcat(fontname, "ExtraBold.ttf");
    }
    else
    {
        strcat(fontname, "Black.ttf");
    }
    FILE* f = fopen(fontname, "rb");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);
        unsigned char* data = (unsigned char*)malloc(len);
        fread(data, 1, len, f);
        fclose(f);
        context->state->textState.fontface = plutovg_font_face_load_from_data1(
            data, len, 0, free, data);
    }
    context->state->textState.font_face_loaded = true;
}
void handle_Tf(pdf_render_t* context)
{
    // set font and font size to use
    // fontname fontsize
    char buf[1024] = { 0 };
    pdf_node_t node;
    node.data = buf;
    pdf_deque_pop_front(context->deque, &node);
    float fontsize = strtof(buf, NULL);
    pdf_deque_pop_front(context->deque, &node);
    printf("font name = %s fontsize = %f\n", (char*)node.data, fontsize);
    pdf_font_t* font = pdf_obj_get_font(context->current_obj, buf);
    context->state->textState.fontSize = fontsize;
    if (font == NULL)
        return;
    
    for (int i = 0; i < context->fontcache.size(); i++)
    {
        pdf_font_cache_t* cache = context->fontcache[i];
        if (cache->font == font)
        {
            context->state->textState.fontface = cache->fontface;
            context->state->textState.font_face_loaded = cache->loaded;
            context->state->textState.font = font;
            //plutovg_canvas_set_font(context->canvas, context->state->textState.fontface, fontsize);
            return;
        }
    }
    
    // plutovg_font_face_destroy(context->state->textState.fontface);
    // plutovg_canvas_set_font_face(context->canvas, NULL);
    // context->state->textState.fontface = NULL;
    context->state->textState.font = font;
    

    // repair font
    // repair_cmap(context->font);
    // set font face
    // context->fontface = plutovg_font_face_load_from_file("fonts/SimSun.ttf",
    // 0); FT_Face face;
    // context->state->textState.font_face_loaded = false;
    // if (font->subtype && strcmp(font->subtype, "/TrueType") == 0)
    // {
    //     if (font->font_data == NULL)
    //     {
    //         context->state->textState.fontface = NULL;
    //         for (int i = 0; i < cvector_size(context->page->pdf->external_fonts); i++)
    //         {
    //             pdf_external_font_t* f = context->page->pdf->external_fonts[i];
    //             if (strcmp(f->name, font->basefont + 1) == 0)
    //             {
    //                 font->font_data_length = f->data_len;
    //                 font->font_data = f->data;
                    
    //                 context->state->textState.fontface = plutovg_font_face_load_from_data(
    //                     font->font_data, font->font_data_length, 0, NULL, NULL);
    //                 context->state->textState.font_face_loaded = true;
    //                 break;
    //             }
    //         }
    //         context->state->textState.font_face_loaded = true;
    //     }
    //     else
    //     {
    //         context->state->textState.fontface = plutovg_font_face_load_from_data(
    //             font->font_data, font->font_data_length, 0, NULL, NULL);
    //         context->state->textState.font_face_loaded = true;
    //     }
    // }
    // else
    if (font->subtype == FONT_SUBTYPE_TYPE0)
    {
        pdf_font_cidfont_t* cidfont = font->type0->descendant->cidfont;
        if (cidfont->font_descriptor->fontfile != NULL)
        {
            if ((context->state->textState.fontface = plutovg_font_face_load_from_data1(
                cidfont->font_descriptor->fontfile, cidfont->font_descriptor->fontfile_len, 0, NULL, NULL)) == NULL)
            {

            }
            else
            {
                context->state->textState.font_face_loaded = true;
            }
        }
        else
        {
            load_font_from_external(context, cidfont->font_descriptor);
        }
    }
    else if (font->subtype == FONT_SUBTYPE_TRUETYPE || font->subtype == FONT_SUBTYPE_TYPE1)
    {
        pdf_font_type1_t* type1_truetype = font->type1_truetype;
        if (type1_truetype->font_descriptor->fontfile != NULL)
        {
            if ((context->state->textState.fontface = plutovg_font_face_load_from_data1(
                type1_truetype->font_descriptor->fontfile, type1_truetype->font_descriptor->fontfile_len, 0, NULL, NULL)) == NULL)
            {

            }
            else
            {
                context->state->textState.font_face_loaded = true;
            }
        }
        else
        {
            load_font_from_external(context, type1_truetype->font_descriptor);
        }
    }
    pdf_font_cache_t* cache = (pdf_font_cache_t*)malloc(sizeof(pdf_font_cache_t));
    cache->font = pdf_font_reference(font);
    cache->fontface = context->state->textState.fontface;
    cache->loaded = context->state->textState.font_face_loaded;
    context->fontcache.push_back(cache);
}