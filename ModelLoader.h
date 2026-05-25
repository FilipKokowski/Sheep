#pragma once
#include <vector>
#include<glm/glm.hpp>

class ModelLoader {

	public:
		static bool loadOBJ(
			const char* path,
			const char* texPath,
			std::vector<glm::vec3>& out_vertices,
			std::vector<glm::vec2>& out_uvs,
			std::vector<glm::vec3>& out_normals,
			unsigned int& tex
		);

};
