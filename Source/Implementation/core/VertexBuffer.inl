#include <string>
#include <list>
#include <memory>
#include <vector>

#include "AttributeNotFoundException.hpp"
#include "BindException.hpp"
#include "Platform.hpp"
#include "ResourceException.hpp"

/// Utility Headers
#include "dynamic_assert.hpp"

/// no-op (here for API consistency)

namespace midnight
{

template<typename T>
VertexBuffer<T>::VertexBuffer(const std::vector<T>& /*data*/)
{

}

template<typename T>
VertexBuffer<T>::VertexBuffer(std::vector<T>&& /*data*/)
{

}

namespace detail
{

    /**
     * Attempts to retrieve an attribute in the provided program with the provided name
     * 
     * @param handle the implementation provided handle to the program in which to search
     * 
     * @param name the name of the attribute to search for
     * 
     * @return A handle to the specified attribute in the specified program
     * 
     * @throw AttributeNotFoundException if no such attribute exists
     * 
     */
    GLint getAttribLocation(GLuint handle, const std::string& name)
    {
        dynamic_assert(handle != 0, "No program is currently bound, and as such attribute lookup has failed");
        /// Call should never fail
        GLint rv = glGetAttribLocation(handle, name.c_str());
        if(rv == -1)
        {
            throw AttributeNotFoundException(std::string("The attribute \"") + name + "\" does not exist");
        }
        return rv;
    }

    /**
     * A helper base-class for encapsulating calls to glVertexAttrib*Pointer.
     * 
     * This class also does most of the (optional) pre-condition checks that the call requires.
     * 
     */
    struct AttributePointerBase
    {
        const std::string name;
        const GLint size;
        const GLenum type;
        const GLsizei stride;
        const GLvoid* offset;
        const GLboolean normalized;

        AttributePointerBase(const std::string& name,
                GLint size,
                GLenum type,
                GLsizei stride,
                const GLvoid* offset,
                GLboolean normalized = GL_FALSE) :
        name(name),
        size(size),
        type(type),
        stride(stride),
        offset(offset),
        normalized(normalized)
        {
            /// Error Condition (1)
            /// GL_INVALID_VALUE is generated if index is greater than or equal to GL_MAX_VERTEX_ATTRIBS
            /// Should never happen - either `index` will be valid, or the attribute will not be found;
            /// thus resulting in an 'AttributeNotFoundException' being thrown.

            /// Error Condition (2)
            /// GL_INVALID_VALUE is generated if size is not 1, 2, 3, 4 or (for glVertexAttribPointer), GL_BGRA
            dynamic_assert((size > 0 && size < 5) || size == GL_BGRA,
                    "size argument must be 1, 2, 3, 4, or GL_BGRA; but " << size << " was provided");

            /// Error Condition (3)
            /// GL_INVALID_ENUM is generated if type is not an accepted value.
            /// Evaluated in each derived AttributePointer type

            /// Error Condition (4)
            /// GL_INVALID_VALUE is generated if stride is negative.
            dynamic_assert(stride >= 0, "stride argument must be positive");


            /// Error Condition (5)
            /// GL_INVALID_OPERATION is generated if size is GL_BGRA and type is not GL_UNSIGNED_BYTE, GL_INT_2_10_10_10_REV or GL_UNSIGNED_INT_2_10_10_10_REV.
            dynamic_assert(
                    size != GL_BGRA ||
                    type == GL_UNSIGNED_BYTE ||
                    type == GL_INT_2_10_10_10_REV ||
                    type == GL_UNSIGNED_INT_2_10_10_10_REV,
                    "type argument must be one of GL_UNSIGNED_BYTE, GL_INT_2_10_10_10_REV or "
                    "GL_UNSIGNED_INT_2_10_10_10_REV if size argument is GL_BGRA; but " << type << " was provided");

            /// Error Condition (6)
            /// GL_INVALID_OPERATION is generated if type is GL_INT_2_10_10_10_REV or GL_UNSIGNED_INT_2_10_10_10_REV and size is not 4 or GL_BGRA.
            dynamic_assert(
                    (type != GL_INT_2_10_10_10_REV && type != GL_UNSIGNED_INT_2_10_10_10_REV) ||
                    size == 4 ||
                    size == GL_BGRA,
                    "size argument must be one of GL_BGRA or '4' if type argument is GL_INT_2_10_10_10_REV or "
                    "GL_UNSIGNED_INT_2_10_10_10_REV; but " << size << " was provided");

            /// Error Condition (7)
            /// GL_INVALID_OPERATION is generated if type is GL_UNSIGNED_INT_10F_11F_11F_REV and size is not 3.
            dynamic_assert(
                    type != GL_UNSIGNED_INT_10F_11F_11F_REV ||
                    size == 3,
                    "size argument must be '3' if type argument is GL_UNSIGNED_INT_10F_11F_11F_REV; but "
                    << size << " was provided");

            /// Error Condition (8)
            /// GL_INVALID_OPERATION is generated by glVertexAttribPointer if size is GL_BGRA and noramlized is GL_FALSE.
            /// I've submitted a typo report for the misspelling of 'normalized' in the official documentation...
            dynamic_assert(
                    size != GL_BGRA ||
                    normalized == GL_TRUE,
                    "normalized argument must be GL_TRUE if size argument is GL_BGRA; but GL_FALSE was provided");

            /// Error Condition (9)
            /// GL_INVALID_OPERATION is generated if zero is bound to the GL_ARRAY_BUFFER buffer object binding point and the pointer argument is not NULL.
            /// Should never happen - A non-zero will always be bound to GL_ARRAY_BUFFER binding point when applying this attribute
        }

        /**
         * Binds this AttributePointer to the implementation state based on the current program
         * 
         */
        void bind(GLuint programHandle)
        {
            GLint location = getAttribLocation(programHandle, name);

            /// Call should never fail
            dynamic_assert(location < GL_MAX_VERTEX_ATTRIBS, "internal error - this should never happen");
            glEnableVertexAttribArray(location);
            postBind(location);
        }

        /**
         * Invoked just after AttributePointerBase::bind is called, this method should make the proper calls 
         * to the implementation dependent on the type of AttributePointer.
         * 
         * @param index the index that this AttributePointer is linked to
         * 
         */
        virtual void postBind(GLint index) = 0;

        /**
         * Unbinds this AttributePointer from the implementation state
         * 
         */
        void unbind()
        {
            GLint handle;

            /// Call should never fail
            glGetIntegerv(GL_CURRENT_PROGRAM, &handle);

            /// Call should never fail
            glDisableVertexAttribArray(getAttribLocation(static_cast<GLuint>(handle), name));
        }

        virtual ~AttributePointerBase() = default;
    };

    /**
     * A helper class for calls to glVertexAttribPointer
     * 
     */
    struct AttributePointer : public AttributePointerBase
    {

        AttributePointer(const std::string& name,
                GLint size,
                GLenum type,
                GLboolean normalized,
                GLsizei stride,
                const GLvoid* offset) : AttributePointerBase(name, size, type, stride, offset, normalized)
        {
            /// Error Condition (3)
            /// GL_INVALID_ENUM is generated if type is not an accepted value.
            dynamic_assert(
                    type == GL_BYTE ||
                    type == GL_UNSIGNED_BYTE ||
                    type == GL_SHORT ||
                    type == GL_UNSIGNED_SHORT ||
                    type == GL_INT ||
                    type == GL_UNSIGNED_INT ||
                    type == GL_HALF_FLOAT ||
                    type == GL_FLOAT ||
                    type == GL_DOUBLE ||
                    type == GL_FIXED ||
                    type == GL_INT_2_10_10_10_REV ||
                    type == GL_UNSIGNED_INT_2_10_10_10_REV ||
                    type == GL_UNSIGNED_INT_10F_11F_11F_REV,
                    "type argument must be one of GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, "
                    "GL_INT, GL_UNSIGNED_INT, GL_HALF_FLOAT, GL_FLOAT, GL_DOUBLE, GL_FIXED, "
                    "GL_INT_2_10_10_10_REV, GL_UNSIGNED_INT_2_10_10_10_REV, or "
                    "GL_UNSIGNED_INT_10F_11F_11F_REV, but " << type << " was provided");
        }

        void postBind(GLint index)
        {
            /// Call should never fail (we've checked all of the preconditions already!)
            glVertexAttribPointer(index, size, type, normalized, stride, offset);
        }
    };

    /**
     * A helper class for calls to glVertexAttribIPointer
     * 
     */
    struct AttributeIPointer : public AttributePointerBase
    {

        AttributeIPointer(const std::string& name,
                GLint size,
                GLenum type,
                GLsizei stride,
                const GLvoid* offset) : AttributePointerBase(name, size, type, stride, offset)
        {
            /// Error Condition (3)
            /// GL_INVALID_ENUM is generated if type is not an accepted value.
            dynamic_assert(
                    type == GL_BYTE ||
                    type == GL_UNSIGNED_BYTE ||
                    type == GL_SHORT ||
                    type == GL_UNSIGNED_SHORT ||
                    type == GL_INT ||
                    type == GL_UNSIGNED_INT,
                    "type argument must be one of GL_BYTE, GL_UNSIGNED_BYTE, GL_SHORT, GL_UNSIGNED_SHORT, "
                    "GL_INT, or GL_UNSIGNED_INT, but " << type << " was provided");
        }

        void postBind(GLint index)
        {
            /// Call should never fail (we've checked all of the preconditions already!)
            glVertexAttribIPointer(index, size, type, stride, offset);
        }
    };

    /**
     * A helper class for calls to glVertexAttribLPointer
     * 
     */
    struct AttributeLPointer : public AttributePointerBase
    {

        AttributeLPointer(const std::string& name,
                GLint size,
                GLenum type,
                GLsizei stride,
                const GLvoid* offset) : AttributePointerBase(name, size, type, stride, offset)
        {
            /// Error Condition (3)
            /// GL_INVALID_ENUM is generated if type is not an accepted value.
            dynamic_assert(
                    type == GL_DOUBLE,
                    "type argument must be GL_DOUBLE, but " << type << " was provided");
        }

        void postBind(GLint index)
        {
            /// Call should never fail (we've checked all of the preconditions already!)
            glVertexAttribLPointer(index, size, type, stride, offset);
        }
    };

    /**
     * A helper class utilizing RAII to bind a VBO to a specific scope
     * 
     */
    struct VertexBufferBindHelper
    {
        GLint preserved;

        VertexBufferBindHelper(GLuint handle)
        {
            /// Call should never fail
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &preserved);

            /// Call should never fail
            glBindBuffer(GL_ARRAY_BUFFER, handle);
        }

        ~VertexBufferBindHelper()
        {
            /// Call should never fail
            glBindBuffer(GL_ARRAY_BUFFER, preserved);
        }
    };

    template<GLenum PolyType>
    struct PolySize
    {
        static constexpr std::size_t size() = delete;
    };

    template<>
    struct PolySize<GL_POINTS>
    {

        static constexpr std::size_t size()
        {
            return 1;
        }
    };

    template<>
    struct PolySize<GL_LINES>
    {

        static constexpr std::size_t size()
        {
            return 2;
        }
    };

    template<>
    struct PolySize<GL_TRIANGLES>
    {

        static constexpr std::size_t size()
        {
            return 3;
        }
    };

    template<>
    struct PolySize<GL_QUADS>
    {

        static constexpr std::size_t size()
        {
            return 4;
        }
    };

    template<typename T, GLenum PolyType, GLenum Usage>
    class VertexBufferImpl : public VertexBuffer<T>
    {
        static_assert(PolyType == GL_TRIANGLES || PolyType == GL_QUADS, "Invalid poly type template provided to VertexBufferImpl");
        /// Quick check to make sure no-one has mucked with our internals...
        static_assert(Usage == GL_STREAM_DRAW ||
                Usage == GL_STREAM_READ ||
                Usage == GL_STREAM_COPY ||
                Usage == GL_STATIC_DRAW ||
                Usage == GL_STATIC_READ ||
                Usage == GL_STATIC_COPY ||
                Usage == GL_DYNAMIC_DRAW ||
                Usage == GL_DYNAMIC_READ ||
                Usage == GL_DYNAMIC_COPY,
                "Invalid intended usage template provided to VertexBufferImpl");

        /// The implementation supplied handle to this buffer object
        GLuint handle;

        /// The data backing this buffer object
        std::vector<T> data;

        /// A list of attribute pointers that are associated with this buffer object
        std::list<std::unique_ptr<detail::AttributePointerBase >> attributes;

        void rebuffer(const std::vector<T>& data)
        {
            GLuint newHandle;
            glGenBuffers(1, &newHandle);
            detail::VertexBufferBindHelper helper(newHandle);
            glBufferData(GL_ARRAY_BUFFER, sizeof(T) * data.size(), &data[0], Usage);
            if(glGetError() == GL_OUT_OF_MEMORY)
            {
                glDeleteBuffers(1, &newHandle);
                throw ResourceException("Unable to allocate GPU memory for VertexBuffer");
            }
            this->data = data;
            glDeleteBuffers(1, &this->handle);
            this->handle = newHandle;
        }

      public:

        VertexBufferImpl(const std::vector<T>& data) : VertexBuffer<T>(data),
        data(data)
        {
            glGenBuffers(1, &handle);
            detail::VertexBufferBindHelper helper(handle);

            /// Can set GL_OUT_OF_MEMORY
            /// https://www.opengl.org/sdk/docs/man4/xhtml/glBufferData.xml
            glBufferData(GL_ARRAY_BUFFER, sizeof(T) * data.size(), &data[0], Usage);

            if(glGetError() == GL_OUT_OF_MEMORY)
            {
                throw ResourceException("Unable to allocate GPU memory for VertexBuffer");
            }
        }

        VertexBufferImpl(std::vector<T>&& data) : VertexBuffer<T>(data), data(data)
        {
            glGenBuffers(1, &handle);
            detail::VertexBufferBindHelper helper(handle);

            /// Can set GL_OUT_OF_MEMORY
            /// https://www.opengl.org/sdk/docs/man4/xhtml/glBufferData.xml
            glBufferData(GL_ARRAY_BUFFER, sizeof(T) * data.size(), &data[0], Usage);

            if(glGetError() == GL_OUT_OF_MEMORY)
            {
                throw ResourceException("Unable to allocate GPU memory for VertexBuffer");
            }
        }

        VertexBufferImpl(VertexBufferImpl&& rhs) :
        VertexBuffer<T>(rhs.data),
        handle(rhs.handle),
        data(std::move(rhs.data)),
        attributes(std::move(rhs.attributes))
        {
            rhs.handle = 0;
        }

        std::size_t vertexCount()
        {
            return data.size() / PolySize<PolyType>::size();
        }

        void setVertexData(const std::vector<T>& data)
        {
            GLint bound;
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bound);
            if(handle == static_cast<GLuint>(bound))
            {
                throw midnight::glsl::BindException("Unable to rebuffer vertex buffer, as it actively bound");
            }
            rebuffer(data);
        }

        void setVertexData(std::vector<T>&& data)
        {
            GLint bound;
            glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &bound);
            if(handle == static_cast<GLuint>(bound))
            {
                throw midnight::glsl::BindException("Unable to rebuffer vertex buffer, as it actively bound");
            }
            rebuffer(data);
        }

        void addAttributePointer(const std::string& name,
                GLint size,
                GLenum type,
                GLboolean normalized,
                GLsizei stride,
                const GLvoid* offset)
        {
            // TODO: C++14 -> replace with std::make_unique(...)
            attributes.push_back(std::move(std::unique_ptr<detail::AttributePointerBase>(new detail::AttributePointer(name, size, type, normalized, stride, offset))));
        }

        void addAttributeIPointer(const std::string& name,
                GLint size,
                GLenum type,
                GLsizei stride,
                const GLvoid* offset)
        {
            // TODO: C++14 -> replace with std::make_unique(...)
            attributes.push_back(std::move(std::unique_ptr<detail::AttributePointerBase>(new detail::AttributeIPointer(name, size, type, stride, offset))));
        }

        void addAttributeLPointer(const std::string& name,
                GLint size,
                GLsizei stride,
                const GLvoid* offset)
        {
            // TODO: C++14 -> replace with std::make_unique(...)
            attributes.push_back(std::move(std::unique_ptr<detail::AttributePointerBase>(new detail::AttributeLPointer(name, size, GL_DOUBLE, stride, offset))));
        }

        void bind()
        {
            GLint programHandle;
            /// Call should never fail
            glGetIntegerv(GL_CURRENT_PROGRAM, &programHandle);
            if(programHandle == 0)
            {
                throw midnight::glsl::BindException("A program must first be bound before binding a VertexBuffer");
            }
            /// Call should never fail
            glBindBuffer(GL_ARRAY_BUFFER, handle);

            /// Apply each of our attributes
            for(std::unique_ptr<detail::AttributePointerBase>& ptr : attributes)
            {
                ptr->bind(static_cast<GLuint>(programHandle));
            }
        }

        void unbind() noexcept
        {
            /// Call should never fail
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            /// Basically, just disable each attribute index that we used
            for(std::unique_ptr<detail::AttributePointerBase>& ptr : attributes)
            {
                ptr->unbind();
            }
        }

        void resetAttributes() noexcept
        {
            attributes.clear();
        }

        ~VertexBufferImpl()
        {
            /// Call should never fail
            glDeleteBuffers(1, &handle);
        }
    };
}

}
