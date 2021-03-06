#include <stdio.h>
#include "NativeTextureGLES.h"
#include "oxgl.h"
#include "MemoryTexture.h"
#include "../oxygine.h"
#include "../NativeTexture.h"
#include "../ImageDataOperations.h"
#include "../file.h"
#include "../log.h"


namespace oxygine
{
	struct glPixel 
	{
		int format;
		int type;
		bool compressed;
	};

	glPixel SurfaceFormat2GL(TextureFormat format)
	{
		glPixel pixel;
		pixel.compressed = false;
		//ADD_SF - dont remove this comment
		switch (format)
		{
		case TF_R8G8B8A8:
			pixel.format = GL_RGBA;
			pixel.type = GL_UNSIGNED_BYTE;
			break;

		case TF_R5G6B5:
			pixel.format = GL_RGB;
			pixel.type = GL_UNSIGNED_SHORT_5_6_5;
			break;

		case TF_R4G4B4A4:
			pixel.format = GL_RGBA;
			pixel.type = GL_UNSIGNED_SHORT_4_4_4_4;
			break;
		
		case TF_R5G5B5A1:
			pixel.format = GL_RGBA;
			pixel.type = GL_UNSIGNED_SHORT_5_5_5_1;
			break;	
		case TF_PVRTC_2RGB:
			pixel.format = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
			pixel.type = 0;
			pixel.compressed = true;
			break;
		case TF_PVRTC_2RGBA:
			pixel.format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
			pixel.type = 0;
			pixel.compressed = true;
			break;
		case TF_PVRTC_4RGB:
			pixel.format = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
			pixel.type = 0;
			pixel.compressed = true;
			break;
		case TF_PVRTC_4RGBA:
			pixel.format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
			pixel.type = 0;
			pixel.compressed = true;
			break;
		case TF_ETC1:
			pixel.format = GL_ETC1_RGB8_OES;
			pixel.type = 0;
			pixel.compressed = true;
			break;
		default:
			log::error("unknown format: %d\n", format);
			OX_ASSERT(!"unknown format");
		}
		return pixel;
	}	

	unsigned int createTexture()
	{
		unsigned int id = 0;
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);

		unsigned int f = GL_LINEAR;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f);

		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

		NativeTextureGLES::created++;
        
        CHECKGL();

		return id;
	}

	NativeTextureGLES::NativeTextureGLES():_id(0), _fbo(0), _width(0), _height(0), _format(TF_UNDEFINED), _lockFlags(0)
	{	

	}

	void NativeTextureGLES::init(int w, int h, TextureFormat tf, bool rt)
	{
		release();


		unsigned int id = createTexture();	

		if (rt)
		{
			w = nextPOT(w);
			h = nextPOT(h);
		}

		glPixel p = SurfaceFormat2GL(tf);
		glTexImage2D(GL_TEXTURE_2D, 0, p.format, w, h, 0, p.format, p.type, 0);
		
		if (rt)
		{
			int prevFBO = 0;
			glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFBO);

			glBindTexture(GL_TEXTURE_2D, 0);

			unsigned int fbo = 0;
			glGenFramebuffers(1, &fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, id, 0);

			//printf("created fbo: %d\n", fbo);

			unsigned int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			//log::message("fbo status %d\n", status);
			//GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES
			if (status != GL_FRAMEBUFFER_COMPLETE)
			{
				log::error("status != GL_FRAMEBUFFER_COMPLETE_OES");
			}

			glViewport(0, 0, w, h);
			glClearColor(0,0,0,0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindFramebuffer(GL_FRAMEBUFFER, prevFBO);
			//log::message("fbo bind\n");

			glBindTexture(GL_TEXTURE_2D, 0);

			_fbo = fbo;
		}			

		_id = id;
		_width = w;
		_height = h;
		_format = tf;
        CHECKGL();
	}

	void NativeTextureGLES::init(nativeTextureHandle id, int w, int h, TextureFormat tf)
	{
		release();
		_id = (unsigned int)id;
		_width = w;
		_height = h;
		_format = tf;
	}

	void NativeTextureGLES::init(const ImageData &src, bool sysMemCopy)
	{
		unsigned int id = createTexture();

		glPixel p = SurfaceFormat2GL(src.format);
		if (p.compressed)		
			glCompressedTexImage2D(GL_TEXTURE_2D, 0, p.format, src.w, src.h, 0, src.h * src.pitch, src.data);
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, p.format, src.w, src.h, 0, p.format, p.type, src.data);			
		}

		_vram = src.h * src.pitch;

		OX_ASSERT(sysMemCopy == false);

		init((nativeTextureHandle)id, src.w, src.h, src.format);
        CHECKGL();
	}

	void NativeTextureGLES::setLinearFiltration(bool enable)
	{
		glBindTexture(GL_TEXTURE_2D, _id);

		unsigned int f = enable ? GL_LINEAR : GL_NEAREST;
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, f);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, f);
        CHECKGL();
	}

	void NativeTextureGLES::release()
	{
		if (_id)
		{
			NativeTextureGLES::created--;
			glDeleteTextures(1, &_id);
			_id = 0;
		}
		

		if (_fbo)
		{
			glDeleteFramebuffers(1, &_fbo);		
			_fbo = 0;
		}

		unreg();
        CHECKGL();
	}


	void NativeTextureGLES::swap(NativeTexture *)
	{

	}

	NativeTextureGLES::~NativeTextureGLES()
	{
		release();
	}

	int NativeTextureGLES::getWidth() const
	{
		return _width;
	}

	int NativeTextureGLES::getHeight() const
	{
		return _height;
	}

	TextureFormat NativeTextureGLES::getFormat() const
	{
		return _format;
	}

	ImageData NativeTextureGLES::lock(lock_flags flags, const Rect *src)
	{
		assert(_lockFlags == 0);


		_lockFlags = flags;
		Rect r(0, 0, _width, _height);

		if (src)
			r = *src;

		OX_ASSERT(r.getX() + r.getWidth() <= _width);
		OX_ASSERT(r.getY() + r.getHeight() <= _height);

		_lockRect = r;

		assert(_lockFlags != 0);

		if (_lockRect.isEmpty())
		{
			OX_ASSERT(!"_lockRect.IsEmpty()");
			return ImageData();
		}

		if (_data.empty())
		{
			//_data.resize(_width)
		}

		ImageData im = 	ImageData(_width, _height, _data.size() / _height, _format, &_data.front());
		return im.getRect(_lockRect);
	}


	void NativeTextureGLES::unlock()
	{
		if (!_lockFlags)
			return;

		if (_lockFlags & lock_write)
		{
			glBindTexture(GL_TEXTURE_2D, _id);
			GLenum er = glGetError();

			ImageData src =	ImageData(_width, _height, _data.size() / _height, _format, &_data.front());
			ImageData locked = src.getRect(_lockRect);

			//glPixelStorei (GL_UNPACK_ALIGNMENT,  1);//byte align
			er = glGetError();

			//todo add EXT_unpack_subimage support

			MemoryTexture mt;
			mt.init(_lockRect.getWidth(), _lockRect.getHeight(), _format);
			ImageData q = mt.lock();
			operations::copy(locked, q);
			mt.unlock();

			glPixel glp = SurfaceFormat2GL(_format);



			glTexSubImage2D(GL_TEXTURE_2D, 0, 
				_lockRect.getX(), _lockRect.getY(), _lockRect.getWidth(), _lockRect.getHeight(), 
				glp.format, glp.type, locked.data);

			er = glGetError();

			_lockFlags = 0;	
		}
        
        CHECKGL();
	}


	void NativeTextureGLES::updateRegion(int x, int y, const ImageData &data)
	{
		assert(_width >= data.w - x);
		assert(_height>= data.h - y);
		glBindTexture(GL_TEXTURE_2D, _id);

		glPixel glp = SurfaceFormat2GL(_format);
		//saveImage(data, "test1.png");

		glTexSubImage2D(GL_TEXTURE_2D, 0, 
			x, y, data.w, data.h, 
			glp.format, glp.type, data.data);
        CHECKGL();
	}

	void NativeTextureGLES::apply(const Rect *)
	{

	}

	nativeTextureHandle NativeTextureGLES::getHandle() const
	{
		return (nativeTextureHandle)_id;
	}

	unsigned int NativeTextureGLES::getFboID() const
	{
		return _fbo;
	}
}