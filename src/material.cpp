#include <material.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>


int loadMaterail(std::string const &path, Material &mat) {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);
    if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        fprintf(stderr, "ERROR::ASSIMP::%s\n",importer.GetErrorString() );
        return -1;
    }
    if (scene->HasMaterials()) {
		
		aiString naziv;
        int i = scene->mNumMaterials - 1;
		scene->mMaterials[i]->Get(AI_MATKEY_NAME, naziv);
		
		aiColor3D ambientK, diffuseK, specularK, reflectiveK;
		float shininessK;

		scene->mMaterials[i]->Get(AI_MATKEY_COLOR_AMBIENT, ambientK);

		scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, diffuseK);

		scene->mMaterials[i]->Get(AI_MATKEY_COLOR_SPECULAR, specularK);

		scene->mMaterials[i]->Get(AI_MATKEY_SHININESS, shininessK);
        
		mat.diffuse = glm::vec3(diffuseK.r,diffuseK.g,diffuseK.b);
        mat.specular = glm::vec3(specularK.r,specularK.g,specularK.b);
        mat.n = shininessK;
        return 1;
    }
    return -1;
}
