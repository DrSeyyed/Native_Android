#ifndef SHADER_H
#define SHADER_H

#include <string>
#include <GLES3/gl3.h>
#include "AndroidOut.h"
#include "Model.h"
#include "Utility.h"

//class Model;

/*!
 * A class representing a simple shader program. It consists of vertex and fragment components. The
 * input attributes are a position (as a Vector3) and a uv (as a Vector2). It also takes a uniform
 * to be used as the entire model/view/projection matrix. The shader expects a single texture for
 * fragment shading, and does no other lighting calculations (thus no uniforms for lights or normal
 * attributes).
 */
class Shader {
public:
    /*!
     * Loads a shader given the full sourcecode and names for necessary attributes and uniforms to
     * link to. Returns a valid shader on success or null on failure. Shader resources are
     * automatically cleaned up on destruction.
     *
     * @param vertexSource The full source code for your vertex program
     * @param fragmentSource The full source code of your fragment program
     * @param positionAttributeName The name of the position attribute in your vertex program
     * @param uvAttributeName The name of the uv coordinate attribute in your vertex program
     * @param projectionMatrixUniformName The name of your model/view/projection matrix uniform
     * @return a valid Shader on success, otherwise null.
     */
    static Shader *loadShader(
            const std::string &vertexSource,
            const std::string &fragmentSource,
            const std::string &positionAttributeName,
            const std::string &uvAttributeName,
            const std::string &projectionMatrixUniformName){
        Shader *shader = nullptr;

        GLuint vertexShader = loadShader(GL_VERTEX_SHADER, vertexSource);
        if (!vertexShader) {
            return nullptr;
        }

        GLuint fragmentShader = loadShader(GL_FRAGMENT_SHADER, fragmentSource);
        if (!fragmentShader) {
            glDeleteShader(vertexShader);
            return nullptr;
        }

        GLuint program = glCreateProgram();
        if (program) {
            glAttachShader(program, vertexShader);
            glAttachShader(program, fragmentShader);

            glLinkProgram(program);
            GLint linkStatus = GL_FALSE;
            glGetProgramiv(program, GL_LINK_STATUS, &linkStatus);
            if (linkStatus != GL_TRUE) {
                GLint logLength = 0;
                glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);

                // If we fail to link the shader program, log the result for debugging
                if (logLength) {
                    GLchar *log = new GLchar[logLength];
                    glGetProgramInfoLog(program, logLength, nullptr, log);
                    aout << "Failed to link program with:\n" << log << std::endl;
                    delete[] log;
                }

                glDeleteProgram(program);
            } else {
                // Get the attribute and uniform locations by name. You may also choose to hardcode
                // indices with layout= in your shader, but it is not done in this sample
                GLint positionAttribute = glGetAttribLocation(program, positionAttributeName.c_str());
                GLint uvAttribute = glGetAttribLocation(program, uvAttributeName.c_str());
                GLint projectionMatrixUniform = glGetUniformLocation(
                        program,
                        projectionMatrixUniformName.c_str());

                // Only create a new shader if all the attributes are found.
                if (positionAttribute != -1
                    && uvAttribute != -1
                    && projectionMatrixUniform != -1) {

                    shader = new Shader(
                            program,
                            positionAttribute,
                            uvAttribute,
                            projectionMatrixUniform);
                } else {
                    glDeleteProgram(program);
                }
            }
        }

        // The shaders are no longer needed once the program is linked. Release their memory.
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        return shader;
    }

    inline ~Shader() {
        if (program_) {
            glDeleteProgram(program_);
            program_ = 0;
        }
    }

    /*!
     * Prepares the shader for use, call this before executing any draw commands
     */
    void activate() const{
        glUseProgram(program_);
    }

    /*!
     * Cleans up the shader after use, call this after executing any draw commands
     */
    void deactivate() const{
        glUseProgram(0);
    }

    /*!
     * Renders a single model
     * @param model a model to render
     */
    void drawModel(const Model &model) const{
        // The position attribute is 3 floats
        glVertexAttribPointer(
                position_, // attrib
                3, // elements
                GL_FLOAT, // of type float
                GL_FALSE, // don't normalize
                sizeof(Vertex), // stride is Vertex bytes
                model.getVertexData() // pull from the start of the vertex data
        );
        glEnableVertexAttribArray(position_);

        // The uv attribute is 2 floats
        glVertexAttribPointer(
                uv_, // attrib
                2, // elements
                GL_FLOAT, // of type float
                GL_FALSE, // don't normalize
                sizeof(Vertex), // stride is Vertex bytes
                ((uint8_t *) model.getVertexData()) + sizeof(Vector3) // offset Vector3 from the start
        );
        glEnableVertexAttribArray(uv_);

        // Setup the texture
        glUniform1i(glGetUniformLocation(program_, "use_RGBA"), model.isRGBA());
        if(model.isRGBA()) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, model.getRGBATexture().getTextureID());
            glUniform1i(glGetUniformLocation(program_, "rgbaTexture"), 0);
        }
        else{
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, model.getYTexture().getTextureID());
            glUniform1i(glGetUniformLocation(program_, "yTexture"), 0);
            glUniform1f(glGetUniformLocation(program_, "yPortion"), model.getYPortion());
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, model.getUTexture().getTextureID());
            glUniform1i(glGetUniformLocation(program_, "uTexture"), 1);
            glUniform1f(glGetUniformLocation(program_, "uPortion"), model.getUPortion());
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, model.getVTexture().getTextureID());
            glUniform1i(glGetUniformLocation(program_, "vTexture"), 2);
            glUniform1f(glGetUniformLocation(program_, "vPortion"), model.getVPortion());
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, model.getATexture().getTextureID());
            glUniform1i(glGetUniformLocation(program_, "aTexture"), 3);
            glUniform1f(glGetUniformLocation(program_, "aPortion"), model.getAPortion());
        }

        // Draw as indexed triangles
        glDrawElements(GL_TRIANGLES, model.getIndexCount(), GL_UNSIGNED_SHORT, model.getIndexData());

        glDisableVertexAttribArray(uv_);
        glDisableVertexAttribArray(position_);
    }

    /*!
     * Sets the model/view/projection matrix in the shader.
     * @param projectionMatrix sixteen floats, column major, defining an OpenGL projection matrix.
     */
    void setProjectionMatrix(float *projectionMatrix) const{
        glUniformMatrix4fv(projectionMatrix_, 1, false, projectionMatrix);
    }

private:
    /*!
     * Helper function to load a shader of a given type
     * @param shaderType The OpenGL shader type. Should either be GL_VERTEX_SHADER or GL_FRAGMENT_SHADER
     * @param shaderSource The full source of the shader
     * @return the id of the shader, as returned by glCreateShader, or 0 in the case of an error
     */
    static GLuint loadShader(GLenum shaderType, const std::string &shaderSource){
        Utility::assertGlError();
        GLuint shader = glCreateShader(shaderType);
        if (shader) {
            auto *shaderRawString = (GLchar *) shaderSource.c_str();
            GLint shaderLength = shaderSource.length();
            glShaderSource(shader, 1, &shaderRawString, &shaderLength);
            glCompileShader(shader);

            GLint shaderCompiled = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &shaderCompiled);

            // If the shader doesn't compile, log the result to the terminal for debugging
            if (!shaderCompiled) {
                GLint infoLength = 0;
                glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLength);

                if (infoLength) {
                    auto *infoLog = new GLchar[infoLength];
                    glGetShaderInfoLog(shader, infoLength, nullptr, infoLog);
                    aout << "Failed to compile with:\n" << infoLog << std::endl;
                    delete[] infoLog;
                }

                glDeleteShader(shader);
                shader = 0;
            }
        }
        return shader;
    }

    /*!
     * Constructs a new instance of a shader. Use @a loadShader
     * @param program the GL program id of the shader
     * @param position the attribute location of the position
     * @param uv the attribute location of the uv coordinates
     * @param projectionMatrix the uniform location of the projection matrix
     */
    constexpr Shader(
            GLuint program,
            GLint position,
            GLint uv,
            GLint projectionMatrix)
            : program_(program),
              position_(position),
              uv_(uv),
              projectionMatrix_(projectionMatrix) {}

    GLuint program_;
    GLint position_;
    GLint uv_;
    GLint projectionMatrix_;
};

#endif