#pragma once

#include <string>
#include <filesystem>
#include <imgui/imgui.h>

namespace fs = std::filesystem;

class Helpers
{
public:

	std::string GetAssetPath(const std::string& relativePath);

	static fs::path FindAssetsRoot();

	// Centers the text horizontally within the current ImGui window and applies a specified color to it.
	void CenterTextColored(const ImVec4& color, const char* text);

private:

};

const ImVec4 COLOR_LIGHTBLUE(0.43f, 0.7f, 0.89f, 1.0f);
const ImVec4 COLOR_LIGHTGREEN(0.0f, 0.9f, 0.0f, 1.0f);
const ImVec4 COLOR_NONE(0.0f, 0.0f, 0.0f, 0.0f);

#define ICON_MIN_FA 0xe005
#define ICON_MAX_16_FA 0xf8ff
#define ICON_MAX_FA 0xf8ff


//const float FONT_SIZE = 26.0f; // comic
const float FONT_SIZE = 12.0f; // comic
const float SMALL_FONT_SIZE = 13.0f;
const float TINY_FONT_SIZE = 10.0f;
const float MY_FONT_SIZE = 18.0f; // my font

constexpr const char* FONT_PATH_MAIN_REL = "assets/fonts/comic.ttf";
constexpr const char* FA_SOLID_PATH = "assets/fonts/FA-Solid-900.otf";

constexpr const char* FONT_PATH_MAIN = "assets/fonts/comic.ttf";

constexpr const char* ICON_PATH = "assets/textures/icons/github.jpg";

