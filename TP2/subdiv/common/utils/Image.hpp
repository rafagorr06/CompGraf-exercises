#ifndef IMAGE_HPP
#define IMAGE_HPP
#include <string>
#include <glm/glm.hpp>

class Image {
public:
	Image() = default;
	Image(const std::string &fname, bool flipY=false);
	Image(const Image &) = delete;
	Image(Image &&other);
	Image &operator=(const Image &) = delete;
	Image &operator=(Image &&other);
	~Image();
	const unsigned char *GetData() const { return m_data; }
	const int GetWidth() const { return m_width; }
	const int GetHeight() const { return m_height; }
	const int GetChannels() const { return m_channels; }
	void SetRGB(int i, int j, const glm::vec3 &rgb);
	void SetRGBA(int i, int j, const glm::vec4 &rgba);
	glm::vec3 GetRGB(int i, int j) const;
	glm::vec4 GetRGBA(int i, int j) const;
private:
	unsigned char *m_data = nullptr;
	int m_width = -1, m_height = -1, m_channels = -1;
	void check_indexes(int i, int j) const;
};

#endif

