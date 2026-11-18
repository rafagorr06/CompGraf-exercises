#include <stb_image.h>
#include "Image.hpp"
#include "Debug.hpp"

Image::Image(const std::string &fname, bool flipY) {
	stbi_set_flip_vertically_on_load(flipY); // tell stb_image.h to flip loaded texture's on the y-axis.
	m_data = stbi_load(fname.c_str(), &m_width, &m_height, &m_channels, 0);
	cg_assert(m_data,"Could not load texture: "+fname);
}

Image::~Image ( ) {
	if (m_data) stbi_image_free(m_data);
}

Image::Image (Image &&other) {
	m_data = other.m_data; other.m_data = nullptr;
	m_width = other.m_width; other.m_width = -1;
	m_height = other.m_height; other.m_height = -1;
	m_channels = other.m_channels; other.m_channels = -1;
}

Image &Image::operator=(Image &&other) {
	if (m_data) stbi_image_free(m_data);
	m_data = other.m_data; other.m_data = nullptr;
	m_width = other.m_width; other.m_width = -1;
	m_height = other.m_height; other.m_height = -1;
	m_channels = other.m_channels; other.m_channels = -1;
	return *this;
}

void Image::SetRGB (int i, int j, const glm::vec3 &rgb) {
	check_indexes(i,j);
	m_data[i*m_width*m_channels+j*m_channels+0] = rgb.r*255;
	m_data[i*m_width*m_channels+j*m_channels+1] = rgb.g*255;
	m_data[i*m_width*m_channels+j*m_channels+2] = rgb.b*255;
	if (m_channels==4) m_data[i*m_width*m_channels+j*m_channels+3] = 255;
}

void Image::SetRGBA (int i, int j, const glm::vec4 &rgba) {
	check_indexes(i,j);
	m_data[i*m_width*m_channels+j*m_channels+0] = rgba.r*255;
	m_data[i*m_width*m_channels+j*m_channels+1] = rgba.g*255;
	m_data[i*m_width*m_channels+j*m_channels+2] = rgba.b*255;
	if (m_channels==4) m_data[i*m_width*m_channels+j*m_channels+3] = rgba.a*255;
}

glm::vec3 Image::GetRGB (int i, int j) const {
	check_indexes(i,j);
	return { m_data[i*m_width*m_channels+j*m_channels+0]/255.f,
		     m_data[i*m_width*m_channels+j*m_channels+1]/255.f,
		     m_data[i*m_width*m_channels+j*m_channels+2]/255.f };
}

glm::vec4 Image::GetRGBA (int i, int j) const {
	check_indexes(i,j);
	return { m_data[i*m_width*m_channels+j*m_channels+0]/255.f,
		     m_data[i*m_width*m_channels+j*m_channels+1]/255.f,
		     m_data[i*m_width*m_channels+j*m_channels+2]/255.f,
		     m_channels==4 ? m_data[i*m_width*m_channels+j*m_channels+3]/255.f : 1.f };
}

void Image::check_indexes (int i, int j) const {
	cg_assert(i>=0 and i<m_height, "Wrong i coord in Image::setRGB");
	cg_assert(j>=0 and i<m_width, "Wrong j coord in Image::setRGB");
}

