#ifndef MODEL_H
#define MODEL_H

#include <vector>
#include "TextureAsset.h"

union Vector3 {
    struct {
        float x, y, z;
    };
    float idx[3];
};

union Vector2 {
    struct {
        float x, y;
    };
    struct {
        float u, v;
    };
    float idx[2];
};

struct Vertex {
    constexpr Vertex(const Vector3 &inPosition, const Vector2 &inUV) : position(inPosition),
                                                                       uv(inUV) {}

    Vector3 position;
    Vector2 uv;
};

typedef uint16_t Index;

class Model {
public:
    inline Model(
            std::vector <Vertex> vertices,
            std::vector <Index> indices,
            std::shared_ptr <TextureAsset> rgbaTexture,
            std::shared_ptr <TextureAsset> yTexture,
            std::shared_ptr <TextureAsset> uTexture,
            std::shared_ptr <TextureAsset> vTexture,
            std::shared_ptr <TextureAsset> aTexture)
            : vertices_(std::move(vertices)),
              indices_(std::move(indices)),
              rgbaTexture_(std::move(rgbaTexture)),
              yTexture_(std::move(yTexture)),
              uTexture_(std::move(uTexture)),
              vTexture_(std::move(vTexture)),
              aTexture_(std::move(aTexture)) {}

    inline Model(
            std::vector <Vertex> vertices,
            std::vector <Index> indices,
            std::shared_ptr <TextureAsset> rgbaTexture,
            std::shared_ptr <TextureAsset> yTexture,
            std::shared_ptr <TextureAsset> uTexture,
            std::shared_ptr <TextureAsset> vTexture,
            std::shared_ptr <TextureAsset> aTexture,
            float y_por, float u_por, float v_por, float a_por)
            : vertices_(std::move(vertices)),
              indices_(std::move(indices)),
              rgbaTexture_(std::move(rgbaTexture)),
              yTexture_(std::move(yTexture)),
              uTexture_(std::move(uTexture)),
              vTexture_(std::move(vTexture)),
              aTexture_(std::move(aTexture)),
              yPortion(y_por),uPortion(u_por),vPortion(v_por),aPortion(a_por){}
    inline const Vertex *getVertexData() const {
        return vertices_.data();
    }
    inline const size_t getIndexCount() const {
        return indices_.size();
    }
    inline const Index *getIndexData() const {
        return indices_.data();
    }
    inline const TextureAsset &getRGBATexture() const {
        return *rgbaTexture_;
    }
    inline const float getYPortion() const{
        return yPortion;
    }
    inline const float getUPortion() const{
        return uPortion;
    }
    inline const float getVPortion() const{
        return vPortion;
    }
    inline const float getAPortion() const{
        return aPortion;
    }
    inline const TextureAsset &getYTexture() const {
        return *yTexture_;
    }
    inline const TextureAsset &getUTexture() const {
        return *uTexture_;
    }
    inline const TextureAsset &getVTexture() const {
        return *vTexture_;
    }
    inline const TextureAsset &getATexture() const {
        return *aTexture_;
    }
    inline bool isRGBA() const{
        if(rgbaTexture_!=nullptr)
            return true;
        return false;
    }

    static Model createModel(std::shared_ptr<TextureAsset> text_asset,float left,float right,float down,float up){
        /*
         * This is a square:
         * 0 --- 1
         * | \   |
         * |  \  |
         * |   \ |
         * 3 --- 2
         */
        std::vector <Vertex> vertices ;
        vertices = {
                Vertex(Vector3{left, up, 0}, Vector2{0, 0}), // 0
                Vertex(Vector3{right, up, 0}, Vector2{1, 0}), // 1
                Vertex(Vector3{right, down, 0}, Vector2{1, 1}), // 2
                Vertex(Vector3{left, down, 0}, Vector2{0, 1}) // 3
        };
        std::vector <Index> indices = {
                0, 1, 2, 0, 2, 3
        };
        // Create a model and put it in the back of the render list.
        return Model(vertices, indices, text_asset, nullptr, nullptr, nullptr, nullptr);
    }
    static Model createYUVModel(std::shared_ptr<TextureAsset> text_asset_y,
                                std::shared_ptr<TextureAsset> text_asset_u,
                                std::shared_ptr<TextureAsset> text_asset_v,
                                std::shared_ptr<TextureAsset> text_asset_a,
                                float left,float right,float down,float up){
        /*
         * This is a square:
         * 0 --- 1
         * | \   |
         * |  \  |
         * |   \ |
         * 3 --- 2
         */
        std::vector <Vertex> vertices ;
        vertices = {
                Vertex(Vector3{left, up, 0}, Vector2{0, 0}), // 0
                Vertex(Vector3{right, up, 0}, Vector2{1, 0}), // 1
                Vertex(Vector3{right, down, 0}, Vector2{1, 1}), // 2
                Vertex(Vector3{left, down, 0}, Vector2{0, 1}) // 3
        };
        std::vector <Index> indices = {
                0, 1, 2, 0, 2, 3
        };
        // Create a model and put it in the back of the render list.
        return Model(vertices, indices, nullptr,text_asset_y,text_asset_u,text_asset_v,text_asset_a);
    }
    static Model createYUVModel_withStride(std::shared_ptr<TextureAsset> text_asset_y,
                                std::shared_ptr<TextureAsset> text_asset_u,
                                std::shared_ptr<TextureAsset> text_asset_v,
                                std::shared_ptr<TextureAsset> text_asset_a,
                                float y_por, float u_por, float v_por,float a_por,
                                float left,float right,float down,float up){
        /*
         * This is a square:
         * 0 --- 1
         * | \   |
         * |  \  |
         * |   \ |
         * 3 --- 2
         */
        std::vector <Vertex> vertices ;
        vertices = {
                Vertex(Vector3{left, up, 0}, Vector2{0, 0}), // 0
                Vertex(Vector3{right, up, 0}, Vector2{1, 0}), // 1
                Vertex(Vector3{right, down, 0}, Vector2{1, 1}), // 2
                Vertex(Vector3{left, down, 0}, Vector2{0, 1}) // 3
        };
        std::vector <Index> indices = {
                0, 1, 2, 0, 2, 3
        };
        // Create a model and put it in the back of the render list.
        return Model(vertices, indices, nullptr,text_asset_y,text_asset_u,text_asset_v,text_asset_a,y_por,u_por,v_por,a_por);
    }

private:
    std::vector <Vertex> vertices_;
    std::vector <Index> indices_;
    std::shared_ptr <TextureAsset> rgbaTexture_;
    std::shared_ptr <TextureAsset> yTexture_;
    std::shared_ptr <TextureAsset> uTexture_;
    std::shared_ptr <TextureAsset> vTexture_;
    std::shared_ptr <TextureAsset> aTexture_;
    float yPortion=1,uPortion=1,vPortion=1,aPortion=1;

};


#endif