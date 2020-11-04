#include "H2645BitstreamParserCore.h"

uint8_t H2645BitstreamParser::Core::_vps[1000];
uint8_t H2645BitstreamParser::Core::_sps[1000];
int32_t H2645BitstreamParser::Core::_vps_size = 0;
int32_t H2645BitstreamParser::Core::_sps_size = 0;

H2645BitstreamParser::Core::Core(void)
{

}

H2645BitstreamParser::Core::~Core(void)
{

}

void H2645BitstreamParser::Core::ParseVideoParameterSet(int32_t video_codec, const uint8_t * vps, int32_t vpssize)
{
    ::memset(_vps, 0x00, sizeof(_vps));
    removeEmulationBytes(_vps, _vps_size, (unsigned char*)vps, vpssize);
    BitVector bv(_vps, 0, 8 * _vps_size);

    unsigned i;
    bv.skipBits(28); // nal_unit_header, vps_video_parameter_set_id, vps_reserved_three_2bits, vps_max_layers_minus1
    unsigned vps_max_sub_layers_minus1 = bv.getBits(3);
    bv.skipBits(17); // vps_temporal_id_nesting_flag, vps_reserved_0xffff_16bits
    ProfileTierLevel(bv, vps_max_sub_layers_minus1);
    bool vps_sub_layer_ordering_info_present_flag = bv.get1BitBoolean();
    for (i = vps_sub_layer_ordering_info_present_flag ? 0 : vps_max_sub_layers_minus1; i <= vps_max_sub_layers_minus1; ++i) 
    {
        (void)bv.get_expGolomb(); // vps_max_dec_pic_buffering_minus1[i]
        (void)bv.get_expGolomb(); // vps_max_num_reorder_pics[i]
        (void)bv.get_expGolomb(); // vps_max_latency_increase_plus1[i]
    }
    unsigned vps_max_layer_id = bv.getBits(6);
    unsigned vps_num_layer_sets_minus1 = bv.get_expGolomb();
    for (i = 1; i <= vps_num_layer_sets_minus1; ++i) 
    {
        bv.skipBits(vps_max_layer_id + 1); // layer_id_included_flag[i][0..vps_max_layer_id]
    }
    bool vps_timing_info_present_flag = bv.get1BitBoolean();
    if (vps_timing_info_present_flag) 
    {
        (void)bv.getBits(32);
        (void)bv.getBits(32);
        //num_units_in_tick = bv.getBits(32);
        //time_scale = bv.getBits(32);
        bool vps_poc_proportional_to_timing_flag = bv.get1BitBoolean();
        if (vps_poc_proportional_to_timing_flag) 
        {
            unsigned vps_num_ticks_poc_diff_one_minus1 = bv.get_expGolomb();
        }
    }
    bool vps_extension_flag = bv.get1BitBoolean();
}

void H2645BitstreamParser::Core::ParseSequenceParameterSet(int32_t video_codec, const uint8_t * sps, int32_t spssize, int32_t & width, int32_t & height)
{
    ::memset(_sps, 0x00, sizeof(_sps));
	removeEmulationBytes(_sps, _sps_size, (unsigned char*)sps, spssize);
	BitVector bv(_sps, 0, 8 * _sps_size);

	if (video_codec== H2645BitstreamParser::VIDEO_CODEC_T::AVC)
	{
		bv.skipBits(8); // forbidden_zero_bit; nal_ref_idc; nal_unit_type
        unsigned profile_idc = bv.getBits(8);
        unsigned constraint_setN_flag = bv.getBits(8); // also "reserved_zero_2bits" at end
        unsigned level_idc = bv.getBits(8);
        unsigned seq_parameter_set_id = bv.get_expGolomb();
        unsigned chroma_format_idc = 1;
        if (profile_idc == 100 || profile_idc == 110 || profile_idc == 122 || profile_idc == 244 || profile_idc == 44 || profile_idc == 83 || profile_idc == 86 || profile_idc == 118 || profile_idc == 128) 
        {
            chroma_format_idc = bv.get_expGolomb();
            if (chroma_format_idc == 3) 
            {
                bool separate_colour_plane_flag = bv.get1BitBoolean();
            }
            (void)bv.get_expGolomb(); // bit_depth_luma_minus8
            (void)bv.get_expGolomb(); // bit_depth_chroma_minus8
            bv.skipBits(1); // qpprime_y_zero_transform_bypass_flag
            bool seq_scaling_matrix_present_flag = bv.get1BitBoolean();
            if (seq_scaling_matrix_present_flag) 
            {
                for (int i = 0; i < ((chroma_format_idc != 3) ? 8 : 12); ++i) 
                {
                    bool seq_scaling_list_present_flag = bv.get1BitBoolean();
                    if (seq_scaling_list_present_flag) 
                    {
                        unsigned sizeOfScalingList = i < 6 ? 16 : 64;
                        unsigned lastScale = 8;
                        unsigned nextScale = 8;
                        for (unsigned j = 0; j < sizeOfScalingList; ++j) 
                        {
                            if (nextScale != 0) 
                            {
                                int delta_scale = bv.get_expGolombSigned();
                                nextScale = (lastScale + delta_scale + 256) % 256;
                            }
                            lastScale = (nextScale == 0) ? lastScale : nextScale;
                        }
                    }
                }
            }
        }
        unsigned log2_max_frame_num_minus4 = bv.get_expGolomb();
        unsigned pic_order_cnt_type = bv.get_expGolomb();
        if (pic_order_cnt_type == 0) 
        {
            unsigned log2_max_pic_order_cnt_lsb_minus4 = bv.get_expGolomb();
        }
        else if (pic_order_cnt_type == 1) 
        {
            bv.skipBits(1); // delta_pic_order_always_zero_flag
            (void)bv.get_expGolombSigned(); // offset_for_non_ref_pic
            (void)bv.get_expGolombSigned(); // offset_for_top_to_bottom_field
            unsigned num_ref_frames_in_pic_order_cnt_cycle = bv.get_expGolomb();
            for (unsigned i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; ++i) 
            {
                (void)bv.get_expGolombSigned(); // offset_for_ref_frame[i]
            }
        }
        unsigned max_num_ref_frames = bv.get_expGolomb();
        bool gaps_in_frame_num_value_allowed_flag = bv.get1BitBoolean();
        unsigned pic_width_in_mbs_minus1 = bv.get_expGolomb();
        unsigned pic_height_in_map_units_minus1 = bv.get_expGolomb();
        bool frame_mbs_only_flag = bv.get1BitBoolean();
        if (!frame_mbs_only_flag) 
        {
            bv.skipBits(1); // mb_adaptive_frame_field_flag
        }
        bv.skipBits(1); // direct_8x8_inference_flag
        bool frame_cropping_flag = bv.get1BitBoolean();
        int left, right, top, bottom;
        int crop_unit_x = 0, crop_unit_y = 0;
        if (frame_cropping_flag) 
        {
            left = bv.get_expGolomb(); // frame_crop_left_offset
            right = bv.get_expGolomb(); // frame_crop_right_offset
            top = bv.get_expGolomb(); // frame_crop_top_offset
            bottom = bv.get_expGolomb(); // frame_crop_bottom_offset
            switch (chroma_format_idc)
            {
            case 0:
                crop_unit_x = 1;
                crop_unit_y = 2 - frame_mbs_only_flag;
                break;
            case 1:
                crop_unit_x = 2;
                crop_unit_y = 2 * (2 - frame_mbs_only_flag);
                break;
            case 2:
                crop_unit_x = 2;
                crop_unit_y = 1 * (2 - frame_mbs_only_flag);
                break;
            case 3:
                crop_unit_x = 1;
                crop_unit_y = 1 * (2 - frame_mbs_only_flag);
                break;
            }
        }

        if (frame_cropping_flag)
        {
            width = (pic_width_in_mbs_minus1 + 1) * 16 - (crop_unit_x * right);
            height = (pic_height_in_map_units_minus1 + 1) * 16 - (crop_unit_y * bottom);
        }
        else
        {
            width = (pic_width_in_mbs_minus1 + 1) * 16;
            height = (pic_height_in_map_units_minus1 + 1) * 16;
        }
        bool vui_parameters_present_flag = bv.get1BitBoolean();
        /*
        if (vui_parameters_present_flag) 
        {
            analyze_vui_parameters(bv, num_units_in_tick, time_scale);
        }
        */
	}
	else if (video_codec == H2645BitstreamParser::VIDEO_CODEC_T::HEVC)
	{
        unsigned i;

        bv.skipBits(16); // nal_unit_header
        bv.skipBits(4); // sps_video_parameter_set_id
        unsigned sps_max_sub_layers_minus1 = bv.getBits(3);
        bv.skipBits(1); // sps_temporal_id_nesting_flag
        ProfileTierLevel(bv, sps_max_sub_layers_minus1);
        (void)bv.get_expGolomb(); // sps_seq_parameter_set_id
        unsigned chroma_format_idc = bv.get_expGolomb();
        if (chroma_format_idc == 3) 
            bv.skipBits(1); // separate_colour_plane_flag
        unsigned pic_width_in_luma_samples = bv.get_expGolomb();
        unsigned pic_height_in_luma_samples = bv.get_expGolomb();
        width = pic_width_in_luma_samples;
        height = pic_height_in_luma_samples;
        bool conformance_window_flag = bv.get1BitBoolean();
        if (conformance_window_flag) 
        {
            unsigned conf_win_left_offset = bv.get_expGolomb();
            unsigned conf_win_right_offset = bv.get_expGolomb();
            unsigned conf_win_top_offset = bv.get_expGolomb();
            unsigned conf_win_bottom_offset = bv.get_expGolomb();
        }
        (void)bv.get_expGolomb(); // bit_depth_luma_minus8
        (void)bv.get_expGolomb(); // bit_depth_chroma_minus8
        unsigned log2_max_pic_order_cnt_lsb_minus4 = bv.get_expGolomb();
        bool sps_sub_layer_ordering_info_present_flag = bv.get1BitBoolean();
        for (i = (sps_sub_layer_ordering_info_present_flag ? 0 : sps_max_sub_layers_minus1); i <= sps_max_sub_layers_minus1; ++i) 
        {
            (void)bv.get_expGolomb(); // sps_max_dec_pic_buffering_minus1[i]
            (void)bv.get_expGolomb(); // sps_max_num_reorder_pics[i]
            (void)bv.get_expGolomb(); // sps_max_latency_increase[i]
        }
        (void)bv.get_expGolomb(); // log2_min_luma_coding_block_size_minus3
        (void)bv.get_expGolomb(); // log2_diff_max_min_luma_coding_block_size
        (void)bv.get_expGolomb(); // log2_min_transform_block_size_minus2
        (void)bv.get_expGolomb(); // log2_diff_max_min_transform_block_size
        (void)bv.get_expGolomb(); // max_transform_hierarchy_depth_inter
        (void)bv.get_expGolomb(); // max_transform_hierarchy_depth_intra
        bool scaling_list_enabled_flag = bv.get1BitBoolean();
        if (scaling_list_enabled_flag) 
        {
            bool sps_scaling_list_data_present_flag = bv.get1BitBoolean();
            if (sps_scaling_list_data_present_flag) 
            {
                // scaling_list_data()
                for (unsigned sizeId = 0; sizeId < 4; ++sizeId) 
                {
                    for (unsigned matrixId = 0; matrixId < (sizeId == 3 ? 2 : 6); ++matrixId) 
                    {
                        bool scaling_list_pred_mode_flag = bv.get1BitBoolean();
                        if (!scaling_list_pred_mode_flag) 
                        {
                            (void)bv.get_expGolomb(); // scaling_list_pred_matrix_id_delta[sizeId][matrixId]
                        }
                        else 
                        {
                            unsigned const c = 1 << (4 + (sizeId << 1));
                            unsigned coefNum = c < 64 ? c : 64;
                            if (sizeId > 1) 
                            {
                                (void)bv.get_expGolomb(); // scaling_list_dc_coef_minus8[sizeId][matrixId]
                            }
                            for (i = 0; i < coefNum; ++i) 
                            {
                                (void)bv.get_expGolomb(); // scaling_list_delta_coef
                            }
                        }
                    }
                }
            }
        }
        bv.skipBits(2); // amp_enabled_flag, sample_adaptive_offset_enabled_flag
        bool pcm_enabled_flag = bv.get1BitBoolean();
        if (pcm_enabled_flag) 
        {
            bv.skipBits(8); // pcm_sample_bit_depth_luma_minus1, pcm_sample_bit_depth_chroma_minus1
            (void)bv.get_expGolomb(); // log2_min_pcm_luma_coding_block_size_minus3
            (void)bv.get_expGolomb(); // log2_diff_max_min_pcm_luma_coding_block_size
            bv.skipBits(1); // pcm_loop_filter_disabled_flag
        }
        unsigned num_short_term_ref_pic_sets = bv.get_expGolomb();
        unsigned num_negative_pics = 0, prev_num_negative_pics = 0;
        unsigned num_positive_pics = 0, prev_num_positive_pics = 0;
        for (i = 0; i < num_short_term_ref_pic_sets; ++i) 
        {
            // short_term_ref_pic_set(i):
            bool inter_ref_pic_set_prediction_flag = false;
            if (i != 0) {
                inter_ref_pic_set_prediction_flag = bv.get1BitBoolean();
            }
            if (inter_ref_pic_set_prediction_flag) 
            {
                if (i == num_short_term_ref_pic_sets) 
                {
                    // This can't happen here, but it's in the spec, so we include it for completeness
                    (void)bv.get_expGolomb(); // delta_idx_minus1
                }
                bv.skipBits(1); // delta_rps_sign
                (void)bv.get_expGolomb(); // abs_delta_rps_minus1
                unsigned NumDeltaPocs = prev_num_negative_pics + prev_num_positive_pics; // correct???
                for (unsigned j = 0; j < NumDeltaPocs; ++j) 
                {
                    bool used_by_curr_pic_flag = bv.get1BitBoolean();
                    if (!used_by_curr_pic_flag) // use_delta_flag[j]
                        bv.skipBits(1);
                }
            }
            else 
            {
                prev_num_negative_pics = num_negative_pics;
                num_negative_pics = bv.get_expGolomb();
                prev_num_positive_pics = num_positive_pics;
                num_positive_pics = bv.get_expGolomb();
                unsigned k;
                for (k = 0; k < num_negative_pics; ++k) 
                {
                    (void)bv.get_expGolomb(); // delta_poc_s0_minus1[k]
                    bv.skipBits(1); // used_by_curr_pic_s0_flag[k]
                }
                for (k = 0; k < num_positive_pics; ++k) 
                {
                    (void)bv.get_expGolomb(); // delta_poc_s1_minus1[k]
                    bv.skipBits(1); // used_by_curr_pic_s1_flag[k]
                }
            }
        }
        bool long_term_ref_pics_present_flag = bv.get1BitBoolean();
        if (long_term_ref_pics_present_flag) 
        {
            unsigned num_long_term_ref_pics_sps = bv.get_expGolomb();
            for (i = 0; i < num_long_term_ref_pics_sps; ++i) 
            {
                bv.skipBits(log2_max_pic_order_cnt_lsb_minus4); // lt_ref_pic_poc_lsb_sps[i]
                bv.skipBits(1); // used_by_curr_pic_lt_sps_flag[1]
            }
        }
        bv.skipBits(2); // sps_temporal_mvp_enabled_flag, strong_intra_smoothing_enabled_flag
        bool vui_parameters_present_flag = bv.get1BitBoolean();
        /*
        if (vui_parameters_present_flag) 
        {
            analyze_vui_parameters(bv, num_units_in_tick, time_scale);
        }
        */
        bool sps_extension_flag = bv.get1BitBoolean();
	}
}

void H2645BitstreamParser::Core::ProfileTierLevel(BitVector & bv, unsigned max_sub_layers_minus1)
{
    bv.skipBits(96);

    unsigned i;
    bool sub_layer_profile_present_flag[7], sub_layer_level_present_flag[7];
    for (i = 0; i < max_sub_layers_minus1; ++i) 
    {
        sub_layer_profile_present_flag[i] = bv.get1BitBoolean();
        sub_layer_level_present_flag[i] = bv.get1BitBoolean();
    }
    if (max_sub_layers_minus1 > 0) 
    {
        bv.skipBits(2 * (8 - max_sub_layers_minus1)); // reserved_zero_2bits
    }
    for (i = 0; i < max_sub_layers_minus1; ++i) 
    {
        if (sub_layer_profile_present_flag[i]) 
        {
            bv.skipBits(88);
        }
        if (sub_layer_level_present_flag[i]) 
        {
            bv.skipBits(8); // sub_layer_level_idc[i]
        }
    }
}