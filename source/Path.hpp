/**
 * \file
 * \author Rudy Castan
 * \date 2025 Fall
 * \par CS200 Computer Graphics I
 * \copyright DigiPen Institute of Technology
 */
#pragma once

#include <filesystem>

namespace assets
{

    std::filesystem::path get_base_path();
    std::filesystem::path locate_asset(const std::filesystem::path& asset_path);
}
