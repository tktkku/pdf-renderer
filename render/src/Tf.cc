#include "pdf-render.h"
#include "pdf-render-private.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb-truetype.h"
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
   info->index_map = 0;
   if (cmap)
   {
        numTables = ttUSHORT(data + cmap + 2);
        
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

pdf_font_face_t* pdf_font_face_load_from_data(const void* data, 
    unsigned int length, int ttcindex, 
    pdf_destroy_func_t destroy_func, void* closure)
{
    stbtt_fontinfo info;
    int offset = stbtt_GetFontOffsetForIndex((unsigned char*)data, ttcindex);
    if (offset == -1 || !stbtt_InitFont_internal1(&info, (unsigned char*)data, offset)) {
        if (destroy_func)
            destroy_func(closure);
        return NULL;
    }
    pdf_font_face_t* face = (pdf_font_face_t*)malloc(sizeof(pdf_font_face_t));
    face->ref_count = 1;
    face->info = info;
    stbtt_GetFontVMetrics(&face->info, &face->ascent, &face->descent, &face->line_gap);
    stbtt_GetFontBoundingBox(&face->info, &face->x1, &face->y1, &face->x2, &face->y2);
    memset(face->glyphs, 0, sizeof(face->glyphs));
    face->destroy_func = destroy_func;
    face->closure = closure;
    return face;
}
pdf_font_face_t* pdf_font_face_reference(pdf_font_face_t* face)
{
    if (face)
        face->ref_count++;
    return face;
}
void pdf_font_face_destroy(pdf_font_face_t* face)
{
    if(face == NULL)
        return;
    if(--face->ref_count == 0) {
        for(int i = 0; i < GLYPH_CACHE_SIZE; i++) {
            if(face->glyphs[i] == NULL)
                continue;
            for(int j = 0; j < GLYPH_CACHE_SIZE; j++) {
                glyph_t* glyph = face->glyphs[i][j];
                if(glyph == NULL)
                    continue;
                stbtt_FreeShape(&face->info, glyph->vertices);
                free(glyph);
            }

            free(face->glyphs[i]);
        }

        if(face->destroy_func)
            face->destroy_func(face->closure);
        free(face);
    }
}

void load_font_from_external(pdf_render* context, const char* basefont, pdf_font_descriptor_t* font_descriptor)
{
    char fontname[256] = { 0 };
    if (basefont)
    {
        if (!strncmp(basefont, "/Helvetica", 10) || !strncmp(basefont, "/Times", 6))
        {
            sprintf(fontname, "fonts/NotoSansSC-Regular.ttf");
            goto LOAD_FONT;
        }
    }
    //TODO
    
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
LOAD_FONT:
    FILE* f = fopen(fontname, "rb");
    if (f)
    {
        fseek(f, 0, SEEK_END);
        long len = ftell(f);
        fseek(f, 0, SEEK_SET);
        unsigned char* data = (unsigned char*)malloc(len);
        fread(data, 1, len, f);
        fclose(f);
        context->state->textState.fontface = pdf_font_face_load_from_data(
            data, len, 0, free, data);
    }
    context->state->textState.font_face_loaded = true;
}
void handle_Tf(pdf_render* context, pdf_render_command* cmd, bool dry_run)
{
    // set font and font size to use
    // fontname fontsize
    
    if (dry_run)
    {
        auto data = context->deque->pop_front();
        float fontsize = strtof(data->data(), NULL);
        auto data2 = context->deque->pop_front();
        // printf("font name = %s fontsize = %f\n", data2->data(), fontsize);
        pdf_font_t* font = pdf_obj_get_font(context->current_obj, data2->data());
        strcpy(font->name, data2->data() + 1);
        int index = -1;
        for (size_t i = 0; i < context->fontcache.size(); i++)
        {
            pdf_font_cache_t* cache = context->fontcache[i];
            if (cache->font == font)
            {
                index = i;
            }
        }
        if (index == -1)
        {
            if (font->subtype == FONT_SUBTYPE_TYPE0)
            {
                pdf_font_cidfont_t* cidfont = font->type0->descendant->cidfont;
                if (cidfont && cidfont->font_descriptor && cidfont->font_descriptor->fontfile != NULL)
                {
                    if ((context->state->textState.fontface = pdf_font_face_load_from_data(
                        cidfont->font_descriptor->fontfile, cidfont->font_descriptor->fontfile_len, 0, NULL, NULL)) == NULL)
                    {
                        load_font_from_external(context, font->basefont, cidfont->font_descriptor);
                    }
                    else
                    {
                        context->state->textState.font_face_loaded = true;
                    }
                }
                else
                {
                    load_font_from_external(context,  font->basefont, cidfont->font_descriptor);
                }
            }
            else if (font->subtype == FONT_SUBTYPE_TRUETYPE || font->subtype == FONT_SUBTYPE_TYPE1)
            {
                pdf_font_type1_t* type1_truetype = font->type1_truetype;
                if (type1_truetype && type1_truetype->font_descriptor && type1_truetype->font_descriptor->fontfile != NULL)
                {
                    if ((context->state->textState.fontface = pdf_font_face_load_from_data(
                        type1_truetype->font_descriptor->fontfile, type1_truetype->font_descriptor->fontfile_len, 0, NULL, NULL)) == NULL)
                    {
                        load_font_from_external(context, font->basefont, type1_truetype->font_descriptor);
                    }
                    else
                    {
                        context->state->textState.font_face_loaded = true;
                    }
                }
                else
                {
                    load_font_from_external(context, font->basefont, type1_truetype->font_descriptor);
                }
            }
            if (context->state->textState.fontface == NULL)
            {
                return;
            }
            pdf_font_cache_t* cache = new pdf_font_cache_t();
            cache->font = pdf_font_reference(font);
            cache->fontface = context->state->textState.fontface;
            cache->loaded = context->state->textState.font_face_loaded;
            context->fontcache.push_back(cache);

            cmd->type = TOKEN_OPERATOR_Tf;
            cmd->Tf.size = fontsize;
            cmd->Tf.font = cache->font;
            cmd->Tf.fontface = cache->fontface;
        }
        else
        {
            pdf_font_cache_t* cache = context->fontcache[index];
            cmd->type = TOKEN_OPERATOR_Tf;
            cmd->Tf.size = fontsize;
            cmd->Tf.font = cache->font;
            cmd->Tf.fontface = cache->fontface;
        }
    }
    else
    {
        context->state->textState.fontSize = cmd->Tf.size;
        context->state->textState.font = cmd->Tf.font;
        context->state->textState.fontface = cmd->Tf.fontface;
    }
}