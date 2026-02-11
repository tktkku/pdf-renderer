#ifndef CMAPS_H
#define CMAPS_H

#include "../pdf-private.h"

#include "Adobe-CNS1-0.h"
#include "Adobe-CNS1-1.h"
#include "Adobe-CNS1-2.h"
#include "Adobe-CNS1-3.h"
#include "Adobe-CNS1-4.h"
#include "Adobe-CNS1-5.h"
#include "Adobe-CNS1-6.h"
#include "Adobe-CNS1-7.h"
#include "B5-H.h"
#include "B5-V.h"
#include "B5pc-H.h"
#include "B5pc-V.h"
#include "CNS-EUC-H.h"
#include "CNS-EUC-V.h"
#include "CNS1-H.h"
#include "CNS1-V.h"
#include "CNS2-H.h"
#include "CNS2-V.h"
#include "ETen-B5-H.h"
#include "ETen-B5-V.h"
#include "ETenms-B5-H.h"
#include "ETenms-B5-V.h"
#include "ETHK-B5-H.h"
#include "ETHK-B5-V.h"
#include "HKdla-B5-H.h"
#include "HKdla-B5-V.h"
#include "HKdlb-B5-H.h"
#include "HKdlb-B5-V.h"
#include "HKgccs-B5-H.h"
#include "HKgccs-B5-V.h"
#include "HKm314-B5-H.h"
#include "HKm314-B5-V.h"
#include "HKm471-B5-H.h"
#include "HKm471-B5-V.h"
#include "HKscs-B5-H.h"
#include "HKscs-B5-V.h"
#include "UniCNS-UCS2-H.h"
#include "UniCNS-UCS2-V.h"
#include "UniCNS-UTF16-H.h"
#include "UniCNS-UTF16-V.h"
#include "UniCNS-UTF32-H.h"
#include "UniCNS-UTF32-V.h"
#include "UniCNS-UTF8-H.h"
#include "UniCNS-UTF8-V.h"
#include "Adobe-GB1-0.h"
#include "Adobe-GB1-1.h"
#include "Adobe-GB1-2.h"
#include "Adobe-GB1-3.h"
#include "Adobe-GB1-4.h"
#include "Adobe-GB1-5.h"
#include "Adobe-GB1-6.h"
#include "GB-EUC-H.h"
#include "GB-EUC-V.h"
#include "GB-H.h"
#include "GB-V.h"
#include "GBK-EUC-H.h"
#include "GBK-EUC-V.h"
#include "GBK2K-H.h"
#include "GBK2K-V.h"
#include "GBKp-EUC-H.h"
#include "GBKp-EUC-V.h"
#include "GBpc-EUC-H.h"
#include "GBpc-EUC-V.h"
#include "GBT-EUC-H.h"
#include "GBT-EUC-V.h"
#include "GBT-H.h"
#include "GBT-V.h"
#include "GBTpc-EUC-H.h"
#include "GBTpc-EUC-V.h"
#include "UniGB-UCS2-H.h"
#include "UniGB-UCS2-V.h"
#include "UniGB-UTF16-H.h"
#include "UniGB-UTF16-V.h"
#include "UniGB-UTF32-H.h"
#include "UniGB-UTF32-V.h"
#include "UniGB-UTF8-H.h"
#include "UniGB-UTF8-V.h"
#include "Identity-H.h"
#include "Identity-V.h"
#include "78-EUC-H.h"
#include "78-EUC-V.h"
#include "78-H.h"
#include "78-RKSJ-H.h"
#include "78-RKSJ-V.h"
#include "78-V.h"
#include "78ms-RKSJ-H.h"
#include "78ms-RKSJ-V.h"
#include "83pv-RKSJ-H.h"
#include "90ms-RKSJ-H.h"
#include "90ms-RKSJ-V.h"
#include "90msp-RKSJ-H.h"
#include "90msp-RKSJ-V.h"
#include "90pv-RKSJ-H.h"
#include "90pv-RKSJ-V.h"
#include "Add-H.h"
#include "Add-RKSJ-H.h"
#include "Add-RKSJ-V.h"
#include "Add-V.h"
#include "Adobe-Japan1-0.h"
#include "Adobe-Japan1-1.h"
#include "Adobe-Japan1-2.h"
#include "Adobe-Japan1-3.h"
#include "Adobe-Japan1-4.h"
#include "Adobe-Japan1-5.h"
#include "Adobe-Japan1-6.h"
#include "Adobe-Japan1-7.h"
#include "EUC-H.h"
#include "EUC-V.h"
#include "Ext-H.h"
#include "Ext-RKSJ-H.h"
#include "Ext-RKSJ-V.h"
#include "Ext-V.h"
#include "H.h"
#include "Hankaku.h"
#include "Hiragana.h"
#include "Katakana.h"
#include "NWP-H.h"
#include "NWP-V.h"
#include "RKSJ-H.h"
#include "RKSJ-V.h"
#include "Roman.h"
#include "UniJIS-UCS2-H.h"
#include "UniJIS-UCS2-HW-H.h"
#include "UniJIS-UCS2-HW-V.h"
#include "UniJIS-UCS2-V.h"
#include "UniJIS-UTF16-H.h"
#include "UniJIS-UTF16-V.h"
#include "UniJIS-UTF32-H.h"
#include "UniJIS-UTF32-V.h"
#include "UniJIS-UTF8-H.h"
#include "UniJIS-UTF8-V.h"
#include "UniJIS2004-UTF16-H.h"
#include "UniJIS2004-UTF16-V.h"
#include "UniJIS2004-UTF32-H.h"
#include "UniJIS2004-UTF32-V.h"
#include "UniJIS2004-UTF8-H.h"
#include "UniJIS2004-UTF8-V.h"
#include "UniJISPro-UCS2-HW-V.h"
#include "UniJISPro-UCS2-V.h"
#include "UniJISPro-UTF8-V.h"
#include "UniJISX0213-UTF32-H.h"
#include "UniJISX0213-UTF32-V.h"
#include "UniJISX02132004-UTF32-H.h"
#include "UniJISX02132004-UTF32-V.h"
#include "V.h"
#include "WP-Symbol.h"
#include "Adobe-Korea1-0.h"
#include "Adobe-Korea1-1.h"
#include "Adobe-Korea1-2.h"
#include "KSC-EUC-H.h"
#include "KSC-EUC-V.h"
#include "KSC-H.h"
#include "KSC-Johab-H.h"
#include "KSC-Johab-V.h"
#include "KSC-V.h"
#include "KSCms-UHC-H.h"
#include "KSCms-UHC-HW-H.h"
#include "KSCms-UHC-HW-V.h"
#include "KSCms-UHC-V.h"
#include "KSCpc-EUC-H.h"
#include "KSCpc-EUC-V.h"
#include "UniKS-UCS2-H.h"
#include "UniKS-UCS2-V.h"
#include "UniKS-UTF16-H.h"
#include "UniKS-UTF16-V.h"
#include "UniKS-UTF32-H.h"
#include "UniKS-UTF32-V.h"
#include "UniKS-UTF8-H.h"
#include "UniKS-UTF8-V.h"
#include "Adobe-KR-0.h"
#include "Adobe-KR-1.h"
#include "Adobe-KR-2.h"
#include "Adobe-KR-3.h"
#include "Adobe-KR-4.h"
#include "Adobe-KR-5.h"
#include "Adobe-KR-6.h"
#include "Adobe-KR-7.h"
#include "Adobe-KR-8.h"
#include "Adobe-KR-9.h"
#include "UniAKR-UTF16-H.h"
#include "UniAKR-UTF32-H.h"
#include "UniAKR-UTF8-H.h"
#include "Adobe-Manga1-0.h"
#include "UniManga-UTF16-H.h"
#include "UniManga-UTF16-V.h"
#include "UniManga-UTF32-H.h"
#include "UniManga-UTF32-V.h"
#include "UniManga-UTF8-H.h"
#include "UniManga-UTF8-V.h"
#include "Adobe-CNS1-UCS2.h"
#include "Adobe-GB1-UCS2.h"
#include "Adobe-Japan1-UCS2.h"
#include "Adobe-Korea1-UCS2.h"
#include "Adobe-KR-UCS2.h"

// Global CMAP index (sorted by name)
static const struct {
    const char *name;
    pdf_cmap_t *cmap;
} g_CMAP_INDEX[194] = {
    { "78-EUC-H", &cmap_78_EUC_H },
    { "78-EUC-V", &cmap_78_EUC_V },
    { "78-H", &cmap_78_H },
    { "78-RKSJ-H", &cmap_78_RKSJ_H },
    { "78-RKSJ-V", &cmap_78_RKSJ_V },
    { "78-V", &cmap_78_V },
    { "78ms-RKSJ-H", &cmap_78MS_RKSJ_H },
    { "78ms-RKSJ-V", &cmap_78MS_RKSJ_V },
    { "83pv-RKSJ-H", &cmap_83PV_RKSJ_H },
    { "90ms-RKSJ-H", &cmap_90MS_RKSJ_H },
    { "90ms-RKSJ-V", &cmap_90MS_RKSJ_V },
    { "90msp-RKSJ-H", &cmap_90MSP_RKSJ_H },
    { "90msp-RKSJ-V", &cmap_90MSP_RKSJ_V },
    { "90pv-RKSJ-H", &cmap_90PV_RKSJ_H },
    { "90pv-RKSJ-V", &cmap_90PV_RKSJ_V },
    { "Add-H", &cmap_ADD_H },
    { "Add-RKSJ-H", &cmap_ADD_RKSJ_H },
    { "Add-RKSJ-V", &cmap_ADD_RKSJ_V },
    { "Add-V", &cmap_ADD_V },
    { "Adobe-CNS1-0", &cmap_ADOBE_CNS1_0 },
    { "Adobe-CNS1-1", &cmap_ADOBE_CNS1_1 },
    { "Adobe-CNS1-2", &cmap_ADOBE_CNS1_2 },
    { "Adobe-CNS1-3", &cmap_ADOBE_CNS1_3 },
    { "Adobe-CNS1-4", &cmap_ADOBE_CNS1_4 },
    { "Adobe-CNS1-5", &cmap_ADOBE_CNS1_5 },
    { "Adobe-CNS1-6", &cmap_ADOBE_CNS1_6 },
    { "Adobe-CNS1-7", &cmap_ADOBE_CNS1_7 },
    { "Adobe-CNS1-UCS2", &cmap_ADOBE_CNS1_UCS2 },
    { "Adobe-GB1-0", &cmap_ADOBE_GB1_0 },
    { "Adobe-GB1-1", &cmap_ADOBE_GB1_1 },
    { "Adobe-GB1-2", &cmap_ADOBE_GB1_2 },
    { "Adobe-GB1-3", &cmap_ADOBE_GB1_3 },
    { "Adobe-GB1-4", &cmap_ADOBE_GB1_4 },
    { "Adobe-GB1-5", &cmap_ADOBE_GB1_5 },
    { "Adobe-GB1-6", &cmap_ADOBE_GB1_6 },
    { "Adobe-GB1-UCS2", &cmap_ADOBE_GB1_UCS2 },
    { "Adobe-Japan1-0", &cmap_ADOBE_JAPAN1_0 },
    { "Adobe-Japan1-1", &cmap_ADOBE_JAPAN1_1 },
    { "Adobe-Japan1-2", &cmap_ADOBE_JAPAN1_2 },
    { "Adobe-Japan1-3", &cmap_ADOBE_JAPAN1_3 },
    { "Adobe-Japan1-4", &cmap_ADOBE_JAPAN1_4 },
    { "Adobe-Japan1-5", &cmap_ADOBE_JAPAN1_5 },
    { "Adobe-Japan1-6", &cmap_ADOBE_JAPAN1_6 },
    { "Adobe-Japan1-7", &cmap_ADOBE_JAPAN1_7 },
    { "Adobe-Japan1-UCS2", &cmap_ADOBE_JAPAN1_UCS2 },
    { "Adobe-KR-0", &cmap_ADOBE_KR_0 },
    { "Adobe-KR-1", &cmap_ADOBE_KR_1 },
    { "Adobe-KR-2", &cmap_ADOBE_KR_2 },
    { "Adobe-KR-3", &cmap_ADOBE_KR_3 },
    { "Adobe-KR-4", &cmap_ADOBE_KR_4 },
    { "Adobe-KR-5", &cmap_ADOBE_KR_5 },
    { "Adobe-KR-6", &cmap_ADOBE_KR_6 },
    { "Adobe-KR-7", &cmap_ADOBE_KR_7 },
    { "Adobe-KR-8", &cmap_ADOBE_KR_8 },
    { "Adobe-KR-9", &cmap_ADOBE_KR_9 },
    { "Adobe-KR-UCS2", &cmap_ADOBE_KR_UCS2 },
    { "Adobe-Korea1-0", &cmap_ADOBE_KOREA1_0 },
    { "Adobe-Korea1-1", &cmap_ADOBE_KOREA1_1 },
    { "Adobe-Korea1-2", &cmap_ADOBE_KOREA1_2 },
    { "Adobe-Korea1-UCS2", &cmap_ADOBE_KOREA1_UCS2 },
    { "Adobe-Manga1-0", &cmap_ADOBE_MANGA1_0 },
    { "B5-H", &cmap_B5_H },
    { "B5-V", &cmap_B5_V },
    { "B5pc-H", &cmap_B5PC_H },
    { "B5pc-V", &cmap_B5PC_V },
    { "CNS-EUC-H", &cmap_CNS_EUC_H },
    { "CNS-EUC-V", &cmap_CNS_EUC_V },
    { "CNS1-H", &cmap_CNS1_H },
    { "CNS1-V", &cmap_CNS1_V },
    { "CNS2-H", &cmap_CNS2_H },
    { "CNS2-V", &cmap_CNS2_V },
    { "ETHK-B5-H", &cmap_ETHK_B5_H },
    { "ETHK-B5-V", &cmap_ETHK_B5_V },
    { "ETen-B5-H", &cmap_ETEN_B5_H },
    { "ETen-B5-V", &cmap_ETEN_B5_V },
    { "ETenms-B5-H", &cmap_ETENMS_B5_H },
    { "ETenms-B5-V", &cmap_ETENMS_B5_V },
    { "EUC-H", &cmap_EUC_H },
    { "EUC-V", &cmap_EUC_V },
    { "Ext-H", &cmap_EXT_H },
    { "Ext-RKSJ-H", &cmap_EXT_RKSJ_H },
    { "Ext-RKSJ-V", &cmap_EXT_RKSJ_V },
    { "Ext-V", &cmap_EXT_V },
    { "GB-EUC-H", &cmap_GB_EUC_H },
    { "GB-EUC-V", &cmap_GB_EUC_V },
    { "GB-H", &cmap_GB_H },
    { "GB-V", &cmap_GB_V },
    { "GBK-EUC-H", &cmap_GBK_EUC_H },
    { "GBK-EUC-V", &cmap_GBK_EUC_V },
    { "GBK2K-H", &cmap_GBK2K_H },
    { "GBK2K-V", &cmap_GBK2K_V },
    { "GBKp-EUC-H", &cmap_GBKP_EUC_H },
    { "GBKp-EUC-V", &cmap_GBKP_EUC_V },
    { "GBT-EUC-H", &cmap_GBT_EUC_H },
    { "GBT-EUC-V", &cmap_GBT_EUC_V },
    { "GBT-H", &cmap_GBT_H },
    { "GBT-V", &cmap_GBT_V },
    { "GBTpc-EUC-H", &cmap_GBTPC_EUC_H },
    { "GBTpc-EUC-V", &cmap_GBTPC_EUC_V },
    { "GBpc-EUC-H", &cmap_GBPC_EUC_H },
    { "GBpc-EUC-V", &cmap_GBPC_EUC_V },
    { "H", &cmap_H },
    { "HKdla-B5-H", &cmap_HKDLA_B5_H },
    { "HKdla-B5-V", &cmap_HKDLA_B5_V },
    { "HKdlb-B5-H", &cmap_HKDLB_B5_H },
    { "HKdlb-B5-V", &cmap_HKDLB_B5_V },
    { "HKgccs-B5-H", &cmap_HKGCCS_B5_H },
    { "HKgccs-B5-V", &cmap_HKGCCS_B5_V },
    { "HKm314-B5-H", &cmap_HKM314_B5_H },
    { "HKm314-B5-V", &cmap_HKM314_B5_V },
    { "HKm471-B5-H", &cmap_HKM471_B5_H },
    { "HKm471-B5-V", &cmap_HKM471_B5_V },
    { "HKscs-B5-H", &cmap_HKSCS_B5_H },
    { "HKscs-B5-V", &cmap_HKSCS_B5_V },
    { "Hankaku", &cmap_HANKAKU },
    { "Hiragana", &cmap_HIRAGANA },
    { "Identity-H", &cmap_IDENTITY_H },
    { "Identity-V", &cmap_IDENTITY_V },
    { "KSC-EUC-H", &cmap_KSC_EUC_H },
    { "KSC-EUC-V", &cmap_KSC_EUC_V },
    { "KSC-H", &cmap_KSC_H },
    { "KSC-Johab-H", &cmap_KSC_JOHAB_H },
    { "KSC-Johab-V", &cmap_KSC_JOHAB_V },
    { "KSC-V", &cmap_KSC_V },
    { "KSCms-UHC-H", &cmap_KSCMS_UHC_H },
    { "KSCms-UHC-HW-H", &cmap_KSCMS_UHC_HW_H },
    { "KSCms-UHC-HW-V", &cmap_KSCMS_UHC_HW_V },
    { "KSCms-UHC-V", &cmap_KSCMS_UHC_V },
    { "KSCpc-EUC-H", &cmap_KSCPC_EUC_H },
    { "KSCpc-EUC-V", &cmap_KSCPC_EUC_V },
    { "Katakana", &cmap_KATAKANA },
    { "NWP-H", &cmap_NWP_H },
    { "NWP-V", &cmap_NWP_V },
    { "RKSJ-H", &cmap_RKSJ_H },
    { "RKSJ-V", &cmap_RKSJ_V },
    { "Roman", &cmap_ROMAN },
    { "UniAKR-UTF16-H", &cmap_UNIAKR_UTF16_H },
    { "UniAKR-UTF32-H", &cmap_UNIAKR_UTF32_H },
    { "UniAKR-UTF8-H", &cmap_UNIAKR_UTF8_H },
    { "UniCNS-UCS2-H", &cmap_UNICNS_UCS2_H },
    { "UniCNS-UCS2-V", &cmap_UNICNS_UCS2_V },
    { "UniCNS-UTF16-H", &cmap_UNICNS_UTF16_H },
    { "UniCNS-UTF16-V", &cmap_UNICNS_UTF16_V },
    { "UniCNS-UTF32-H", &cmap_UNICNS_UTF32_H },
    { "UniCNS-UTF32-V", &cmap_UNICNS_UTF32_V },
    { "UniCNS-UTF8-H", &cmap_UNICNS_UTF8_H },
    { "UniCNS-UTF8-V", &cmap_UNICNS_UTF8_V },
    { "UniGB-UCS2-H", &cmap_UNIGB_UCS2_H },
    { "UniGB-UCS2-V", &cmap_UNIGB_UCS2_V },
    { "UniGB-UTF16-H", &cmap_UNIGB_UTF16_H },
    { "UniGB-UTF16-V", &cmap_UNIGB_UTF16_V },
    { "UniGB-UTF32-H", &cmap_UNIGB_UTF32_H },
    { "UniGB-UTF32-V", &cmap_UNIGB_UTF32_V },
    { "UniGB-UTF8-H", &cmap_UNIGB_UTF8_H },
    { "UniGB-UTF8-V", &cmap_UNIGB_UTF8_V },
    { "UniJIS-UCS2-H", &cmap_UNIJIS_UCS2_H },
    { "UniJIS-UCS2-HW-H", &cmap_UNIJIS_UCS2_HW_H },
    { "UniJIS-UCS2-HW-V", &cmap_UNIJIS_UCS2_HW_V },
    { "UniJIS-UCS2-V", &cmap_UNIJIS_UCS2_V },
    { "UniJIS-UTF16-H", &cmap_UNIJIS_UTF16_H },
    { "UniJIS-UTF16-V", &cmap_UNIJIS_UTF16_V },
    { "UniJIS-UTF32-H", &cmap_UNIJIS_UTF32_H },
    { "UniJIS-UTF32-V", &cmap_UNIJIS_UTF32_V },
    { "UniJIS-UTF8-H", &cmap_UNIJIS_UTF8_H },
    { "UniJIS-UTF8-V", &cmap_UNIJIS_UTF8_V },
    { "UniJIS2004-UTF16-H", &cmap_UNIJIS2004_UTF16_H },
    { "UniJIS2004-UTF16-V", &cmap_UNIJIS2004_UTF16_V },
    { "UniJIS2004-UTF32-H", &cmap_UNIJIS2004_UTF32_H },
    { "UniJIS2004-UTF32-V", &cmap_UNIJIS2004_UTF32_V },
    { "UniJIS2004-UTF8-H", &cmap_UNIJIS2004_UTF8_H },
    { "UniJIS2004-UTF8-V", &cmap_UNIJIS2004_UTF8_V },
    { "UniJISPro-UCS2-HW-V", &cmap_UNIJISPRO_UCS2_HW_V },
    { "UniJISPro-UCS2-V", &cmap_UNIJISPRO_UCS2_V },
    { "UniJISPro-UTF8-V", &cmap_UNIJISPRO_UTF8_V },
    { "UniJISX0213-UTF32-H", &cmap_UNIJISX0213_UTF32_H },
    { "UniJISX0213-UTF32-V", &cmap_UNIJISX0213_UTF32_V },
    { "UniJISX02132004-UTF32-H", &cmap_UNIJISX02132004_UTF32_H },
    { "UniJISX02132004-UTF32-V", &cmap_UNIJISX02132004_UTF32_V },
    { "UniKS-UCS2-H", &cmap_UNIKS_UCS2_H },
    { "UniKS-UCS2-V", &cmap_UNIKS_UCS2_V },
    { "UniKS-UTF16-H", &cmap_UNIKS_UTF16_H },
    { "UniKS-UTF16-V", &cmap_UNIKS_UTF16_V },
    { "UniKS-UTF32-H", &cmap_UNIKS_UTF32_H },
    { "UniKS-UTF32-V", &cmap_UNIKS_UTF32_V },
    { "UniKS-UTF8-H", &cmap_UNIKS_UTF8_H },
    { "UniKS-UTF8-V", &cmap_UNIKS_UTF8_V },
    { "UniManga-UTF16-H", &cmap_UNIMANGA_UTF16_H },
    { "UniManga-UTF16-V", &cmap_UNIMANGA_UTF16_V },
    { "UniManga-UTF32-H", &cmap_UNIMANGA_UTF32_H },
    { "UniManga-UTF32-V", &cmap_UNIMANGA_UTF32_V },
    { "UniManga-UTF8-H", &cmap_UNIMANGA_UTF8_H },
    { "UniManga-UTF8-V", &cmap_UNIMANGA_UTF8_V },
    { "V", &cmap_V },
    { "WP-Symbol", &cmap_WP_SYMBOL },
};

#endif // CMAPS_H
