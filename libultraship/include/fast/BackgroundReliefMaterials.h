// Reviewed material policies; source: Records/background-materials/material-profiles.json.
#pragma once
#include <string_view>
namespace Fast {
struct BackgroundReliefMaterial { const char* path; float normal, parallax; bool invertHeight=false; int heightMode=0; };
inline constexpr BackgroundReliefMaterial kBackgroundReliefMaterials[] = {
    {"alt/scenes/shared/alley_shop_scene/rm_drag01_croth_2A80", 0.30f, 0.00f, false, 2}, // plaster
    {"alt/scenes/shared/alley_shop_scene/rm_drag01_jyuta_4A80", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/alley_shop_scene/rm_drag01_jyuta_6A80", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/alley_shop_scene/rm_drag01_yuka__13280", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/alley_shop_scene/rm_drag01_yuka__15280", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/alley_shop_scene/sh_maho_ki_01_1F280", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/alley_shop_scene/shop_drag_01_ka_22780", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/drag_scene/rm_naga_tebl_01_3A80", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/drag_scene/sh_drag_02_kabe_8280", 0.35f, 0.15f, true, 0}, // plaster
    {"alt/scenes/shared/drag_scene/sh_drag_kabe_01_1D280", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/drag_scene/sh_drag_kabe_02_1F280", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/drag_scene/sh_drag_ten_01_36280", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/drag_scene/sh_drag_yuka_01_38280", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/drag_scene/sh_maho_ki_01_3A280", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/enrui_scene/hairal_hasi_01__80", 0.00f, 0.00f, false, 4}, // wood
    {"alt/scenes/shared/enrui_scene/hairal_kabe_02__2A80", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/enrui_scene/hairal_ki_01_r_5480", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/enrui_scene/hairal_siro_01__11C80", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/enrui_scene/hairal_siro_02__15C80", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/enrui_scene/hairal_siro_03__18680", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/enrui_scene/hairal_siro_04__1B080", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/enrui_scene/hairal_siro_05__1C580", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/enrui_scene/hairal_yuka_01__25E80", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/enrui_scene/hairal_yuka_02__28880", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/entra_n_scene/hairal_hasi_01_80", 0.00f, 0.00f, false, 4}, // wood
    {"alt/scenes/shared/entra_n_scene/hairal_kabe_01_2A80", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/entra_n_scene/hairal_kabe_02_5480", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/entra_n_scene/hairal_ki_01_BE80", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/entra_n_scene/hairal_siro_01_1B280", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/entra_n_scene/hairal_siro_02_20680", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/entra_n_scene/hairal_siro_03_23080", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/entra_n_scene/hairal_siro_04_25A80", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/entra_n_scene/hairal_siro_05_26F80", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/entra_n_scene/hairal_yuka_01_33E80", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/entra_n_scene/hairal_yuka_02_36880", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/entra_scene/hairal_hasi_01_80", 0.00f, 0.00f, false, 4}, // wood
    {"alt/scenes/shared/entra_scene/hairal_kabe_01_2A80", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/entra_scene/hairal_kabe_02_5480", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/entra_scene/hairal_ki_01_BE80", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/entra_scene/hairal_saku_04_19E80", 0.00f, 0.00f, false, 4}, // wood
    {"alt/scenes/shared/entra_scene/hairal_siro_01_1B280", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/entra_scene/hairal_siro_02_20680", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/entra_scene/hairal_siro_03_23080", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/entra_scene/hairal_siro_04_25A80", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/entra_scene/hairal_siro_05_26F80", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/entra_scene/hairal_yuka_01_33E80", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/entra_scene/hairal_yuka_02_36880", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/face_shop_scene/sh_face_dontyou_10000", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/face_shop_scene/sh_face_jyutan__12000", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/face_shop_scene/sh_face_kabe_01_16000", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/face_shop_scene/sh_face_kabe_03_18000", 0.00f, 0.00f, false, 4}, // plaster
    {"alt/scenes/shared/face_shop_scene/sh_face_yuka_01_20A00", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/golon_scene/sh_gl_box_01_0", 0.00f, 0.00f, false, 4}, // wood
    {"alt/scenes/shared/golon_scene/sh_gl_kabe_01_3000", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/golon_scene/sh_gl_kabe_02_D800", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/golon_scene/sh_gl_table_01_1E000", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/golon_scene/sh_gl_yuka_01_25E00", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/golon_scene/sh_gl_yuka_02_28800", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/hut_scene/rm_dp_kabe_01_4000", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/hut_scene/rm_dp_mado_01_AE00", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/hut_scene/rm_dp_poal_01_19200", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/hut_scene/rm_dp_table_01_1BA00", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/hut_scene/rm_dp_yuka_01_1DA00", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/impa_scene/rm_hairal_01_jy_B000", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/impa_scene/rm_hairal_01_jy_F000", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/impa_scene/rm_hairal_01_ka_11000", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/impa_scene/rm_hairal_01_te_40000", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/impa_scene/rm_hairal_01_yu_42000", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/kakariko3_scene/rm_hairal_02_jy_6000", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/kakariko3_scene/rm_hairal_02_ka_8000", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/kakariko3_scene/rm_hairal_02_te_22800", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/kakariko3_scene/rm_hairal_02_yu_2A800", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/kakariko_scene/rm_naga_bed_01_19780", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/kakariko_scene/rm_naga_bed_03_1EB80", 0.00f, 0.00f, false, 4}, // wood
    {"alt/scenes/shared/kakariko_scene/rm_naga_jyutan__2A780", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/kakariko_scene/rm_naga_jyutan__2C780", 0.00f, 0.00f, false, 4}, // cloth
    {"alt/scenes/shared/kakariko_scene/rm_naga_kabe_01_2E780", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/kakariko_scene/rm_naga_kabek_0_31180", 0.00f, 0.00f, false, 4}, // cloth
    {"alt/scenes/shared/kakariko_scene/rm_naga_renga_0_4A280", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/kakariko_scene/rm_naga_tebl_01_4C280", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/kakariko_scene/rm_naga_yuka_01_4D780", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_home3_scene/kok_rm_kabe_02_80", 0.55f, 0.20f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_home3_scene/kok_rm_kabe_03_4080", 0.55f, 0.20f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_home3_scene/kok_rm_yuka_01_2B000", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_home3_scene/kok_rm_yuka_02_22080", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/kokiri_home3_scene/kok_rm_yuka_03_2A080", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_home4_scene/kok_rm_kabe_02_80", 0.55f, 0.20f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_home4_scene/kok_rm_yuka_01_2B000", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_home4_scene/kok_rm_yuka_04_1E080", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/kokiri_home4_scene/kok_rm_yuka_05_20A80", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/kokiri_home5_scene/kok_rm_kabe_02_80", 0.55f, 0.20f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_home5_scene/kok_rm_obj_06_23480", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/kokiri_home5_scene/kok_rm_yuka_01_2B000", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_home5_scene/kok_rm_yuka_03_25480", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_home_scene/kok_rm_kabe_01_5C00", 0.55f, 0.20f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_home_scene/kok_rm_yuka_01_2B000", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_home_scene/kok_rm_yuka_02_33000", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/kokiri_shop_scene/kok_rm_kabe_02_0", 0.55f, 0.20f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_shop_scene/kok_rm_kabe_04_4000", 0.55f, 0.20f, false, 1}, // wood
    {"alt/scenes/shared/kokiri_shop_scene/kok_rm_yuka_02_22000", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/kokiri_shop_scene/kok_shop_yuka_0_34800", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/labo_scene/komono_02_2A00", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/labo_scene/rm_impa_croth_0_7400", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/labo_scene/rm_impa_jyutan__A900", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/labo_scene/rm_impa_kabe_01_C900", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/labo_scene/rm_impa_kabe_02_E900", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/labo_scene/rm_impa_ten_01_31B00", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/labo_scene/rm_impa_yuka_01_3BB00", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/labo_scene/rm_impa_yuka_02_3DB00", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/link_home_scene/kok_rm_kabe_01_5800", 0.55f, 0.20f, false, 1}, // wood
    {"alt/scenes/shared/link_home_scene/kok_rm_yuka_01_38800", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/malon_stable_scene/stb_door_01_0", 0.00f, 0.00f, false, 4}, // wood
    {"alt/scenes/shared/malon_stable_scene/stb_kabe_01_A800", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/malon_stable_scene/stb_poal_01_2D200", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/malon_stable_scene/stb_poal_02_2FC00", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/malon_stable_scene/stb_yane_01_33B00", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/malon_stable_scene/stb_yuka_01_36500", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/malon_stable_scene/stb_yuka_02_38F00", 0.30f, 0.00f, false, 2}, // cloth
    {"alt/scenes/shared/market_alley_n_scene/hairal_kabe_01__UDIM_", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/market_alley_n_scene/hairal_kabe_02_3F00", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/market_alley_n_scene/hairal_kabe_02b_6900", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_alley_n_scene/hairal_kabe_04_9300", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/market_alley_n_scene/hairal_kabe_04b_B300", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/market_alley_n_scene/hairal_yuka_01_3AF00", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/market_alley_n_scene/hairal_yuka_02_3D900", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_alley_scene/hairal_kabe_01__UDIM_", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/market_alley_scene/hairal_kabe_02_3F00", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/market_alley_scene/hairal_kabe_02b_6900", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_alley_scene/hairal_kabe_04_9300", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/market_alley_scene/hairal_kabe_04b_B300", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/market_alley_scene/hairal_yuka_01_3AF00", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/market_alley_scene/hairal_yuka_02_3D900", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_day_scene/hairal_kabe_01_4880", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/market_day_scene/hairal_kabe_02_7280", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/market_day_scene/hairal_kabe_02b_9C80", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_day_scene/hairal_kabe_04_D080", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/market_day_scene/hairal_kabe_04b_F080", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/market_day_scene/hairal_ki_01_10080", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/market_day_scene/hairal_siro_03_62480", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/market_day_scene/hairal_siro_06_64E80", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/market_day_scene/hairal_yuka_01_73F00", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/market_day_scene/hairal_yuka_01b_76900", 0.90f, 0.65f, true, 8}, // stone
    {"alt/scenes/shared/market_day_scene/hairal_yuka_01c_79300", 0.90f, 0.65f, true, 8}, // stone
    {"alt/scenes/shared/market_day_scene/hairal_yuka_01d_7BD00", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/market_day_scene/hairal_yuka_02_7D200", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_day_scene/shr_en_kabe_7FE00", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_night_scene/hairal_kabe_01_4880", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/market_night_scene/hairal_kabe_02_7280", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/market_night_scene/hairal_kabe_02b_9C80", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_night_scene/hairal_kabe_04_D080", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/market_night_scene/hairal_kabe_04b_F080", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/market_night_scene/hairal_ki_01_10080", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/market_night_scene/hairal_siro_03_62480", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_night_scene/hairal_siro_06_64E80", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/market_night_scene/hairal_yuka_01_73F00", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/market_night_scene/hairal_yuka_01b_76900", 0.90f, 0.65f, true, 8}, // stone
    {"alt/scenes/shared/market_night_scene/hairal_yuka_01c_79300", 0.90f, 0.65f, true, 8}, // stone
    {"alt/scenes/shared/market_night_scene/hairal_yuka_01d_7BD00", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/market_night_scene/hairal_yuka_02_7D200", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_night_scene/shr_en_kabe_7FE00", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/hairal_door_01__4480", 0.00f, 0.00f, false, 4}, // wood
    {"alt/scenes/shared/market_ruins_scene/hairal_kabe_01__8400", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/hairal_kabe_02__D800", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/hairal_kabe_02b_10200", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/hairal_kabe_04__13680", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/hairal_kabe_04__16080", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/hairal_ki_01_r_1B480", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/market_ruins_scene/hairal_siro_03__34F80", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/market_ruins_scene/hairal_yuka_01__41F80", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/hairal_yuka_01b_44980", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/hairal_yuka_01c_47380", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/hairal_yuka_01d_49D80", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/market_ruins_scene/hairal_yuka_02__4B280", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/hairal_yuka_04__4DE80", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/shr_en_kabe_r_50880", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/shr_en_yuka_r_53280", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/shr_kabe_01_r_55C80", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/market_ruins_scene/shr_kad_01_r_5DA80", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/market_ruins_scene/shr_siro_01_r_61980", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/night_shop_scene/hi_rm_kabe_04_E000", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/night_shop_scene/hi_rm_yuka_02_10000", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/night_shop_scene/shop_bomchu_kab_15000", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/night_shop_scene/shop_bomchu_ki__1B000", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/night_shop_scene/shop_bomchu_ten_29000", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/shop1_scene/shop_any_box_01_A80", 0.00f, 0.00f, false, 4}, // wood
    {"alt/scenes/shared/shop1_scene/shop_any_kabe_0_4A80", 0.35f, 0.00f, false, 3}, // plaster
    {"alt/scenes/shared/shop1_scene/shop_any_kabe_0_6A80", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/shop1_scene/shop_any_ten_01_22280", 0.50f, 0.15f, false, 1}, // wood
    {"alt/scenes/shared/shop1_scene/shop_any_yuka_0_24280", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/shrine_n_scene/shr_en_kabe_0", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/shrine_n_scene/shr_ieyane_01_6900", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/shrine_n_scene/shr_kabe_01_7D00", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/shrine_n_scene/shr_kad_01_F900", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/shrine_n_scene/shr_yuka_01_24600", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/shrine_n_scene/shr_yuka_03_34200", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/shrine_r_scene/shr_en_kabe_0", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/shrine_r_scene/shr_en_yuka_2A00", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/shrine_r_scene/shr_ieyane_01_5400", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/shrine_r_scene/shr_kabe_01_7E00", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/shrine_r_scene/shr_kad_01_FC00", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/shrine_r_scene/shr_siro_01_16F80", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/shrine_r_scene/shr_yuka_01_2B100", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/shrine_r_scene/shr_yuka_03_3AD00", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/shrine_scene/shr_en_kabe_0", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/shrine_scene/shr_ieyane_01_6900", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/shrine_scene/shr_kabe_01_7D00", 0.90f, 0.65f, true, 0}, // stone
    {"alt/scenes/shared/shrine_scene/shr_kad_01_F900", 0.00f, 0.00f, false, 4}, // stone
    {"alt/scenes/shared/shrine_scene/shr_yuka_01_24600", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/shrine_scene/shr_yuka_03_34200", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/tent_scene/rm_ten_kabe_01_0", 0.00f, 0.00f, false, 4}, // cloth
    {"alt/scenes/shared/tent_scene/rm_ten_ten_01_15380", 0.00f, 0.00f, false, 4}, // cloth
    {"alt/scenes/shared/tent_scene/rm_ten_yuka_01_1FB80", 0.00f, 0.00f, false, 4}, // cloth
    {"alt/scenes/shared/tent_scene/rm_ten_yuka_02_2A380", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/zoora_scene/sh_zr_kabe_01_0", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/zoora_scene/sh_zr_table_01_8000", 0.90f, 0.65f, false, 0}, // stone
    {"alt/scenes/shared/zoora_scene/sh_zr_yuka_01_D200", 0.90f, 0.65f, false, 0}, // stone
};
// Reviewed Hyrule Field grass/soil, rock and opaque bark resources.
inline constexpr BackgroundReliefMaterial kFieldReliefMaterials[] = {
    {"alt/objects/object_wood02/object_wood02_Tex_000790", 0.65f, 0.30f, false, 1}, // bark
    {"alt/objects/object_wood02/object_wood02_Tex_002F90", 0.65f, 0.30f, false, 1}, // bark
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_014998", 0.85f, 0.40f, false, 7}, // masonry
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_016B98", 1.10f, 0.60f, false, 7}, // cliff
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_018B98", 1.00f, 0.40f, false, 6}, // dirt
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_019B98", 1.10f, 0.60f, false, 7}, // cliff
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_01AB98", 1.20f, 0.35f, false, 6}, // grass
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_01B398", 1.20f, 0.35f, false, 6}, // grass
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_01D998", 0.80f, 0.00f, false, 7}, // cliff edge - normals only
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_020018", 0.65f, 0.30f, false, 1}, // bark
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_021818", 0.85f, 0.40f, false, 7}, // masonry
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_022818", 0.75f, 0.35f, false, 7}, // stone bridge
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_023018", 0.85f, 0.40f, false, 7}, // stonework
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_025818", 0.85f, 0.40f, false, 7}, // masonry
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_025898", 0.65f, 0.30f, false, 1}, // bark
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_026098", 0.50f, 0.20f, false, 1}, // stump rings
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_029D98", 0.85f, 0.40f, false, 7}, // masonry
    {"alt/scenes/shared/spot00_scene/spot00_sceneTex_029F98", 0.85f, 0.40f, false, 7}, // masonry
};
inline const BackgroundReliefMaterial* FieldRelief(std::string_view path) {
    for (const auto& material : kFieldReliefMaterials) if (path==material.path) return &material;
    return nullptr;
}
inline const BackgroundReliefMaterial* BackgroundRelief(std::string_view path) {
    if (const auto* field=FieldRelief(path)) return field;
    for (const auto& material : kBackgroundReliefMaterials) if (path == material.path) return &material;
    // Authored for the active Reloaded half-door texture; shared across scenes.
    if (path=="alt/objects/gameplay_field_keep/gFieldDoor1Tex") {
        static const BackgroundReliefMaterial door={"alt/objects/gameplay_field_keep/gFieldDoor1Tex",2.0f,2.0f,false,5};
        return &door;
    }
    return nullptr;
}
} // namespace Fast
