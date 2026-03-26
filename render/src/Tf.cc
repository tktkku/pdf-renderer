#include "pdf-render.h"
#include "pdf-render-private.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb-truetype.h"

pdf_font_face_t* pdf_font_face_load_from_data(const void* data, 
    unsigned int length, int ttcindex, 
    pdf_destroy_func_t destroy_func, void* closure)
{
    stbtt_fontinfo info;
    int offset = stbtt_GetFontOffsetForIndex((unsigned char*)data, ttcindex);
    if (offset == -1 || !stbtt_InitFont_internal(&info, (unsigned char*)data, offset)) {
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
        if (!strcmp(basefont, "/Helvetica"))
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