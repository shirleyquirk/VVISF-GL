#ifndef ISFScene_hpp
#define ISFScene_hpp

#include "VVISF_Base.hpp"
#if defined(VVGL_SDK_MAC)
#import <TargetConditionals.h>
#endif




namespace VVISF
{


using namespace std;




//! Subclass of GLScene- the primary interface for rendering and interacting with an ISF file.
/*!
\ingroup VVISF_BASIC
Notes on use:
- You should strive to work with ISFSceneRef whenever possible.
- You should avoid calling setVertexShaderString() or setFragmentShaderString() directly on instances of ISFScene- instead, let the class populate its own shaders.
*/
class VVISF_EXPORT ISFScene : public GLScene	{
	private:
		bool			_throwExceptions = false;	//	NO by default

		mutex			_propertyLock;	//	locks the below two vars
		//bool			loadingInProgress = false;
		ISFDocRef		_doc = nullptr;	//	the ISFDoc being used

		//	access to these vars should be restricted by the '_renderLock' var inherited from GLScene
		VVGL::Size		_renderSize = _orthoSize;	//	the last size at which i was requested to render a buffer (used to produce vals from normalized point inputs that need a render size to be used)
		Timestamp		_baseTime;	//	used to generate time values, some of which get passed to the ISF scene
		uint32_t		_renderFrameIndex = 0;	//	used to pass FRAMEINDEX to shaders
		double			_renderTime = 0.;	//	this is the render time that gets passed to the ISF
		double			_renderTimeDelta = 0.;	//	this is the render time delta (frame duration) which gets passed to the ISF
		uint32_t		_passIndex = 1;	//	used to store the index of the rendered pass, which gets passed to the shader
		string			*_compiledInputTypeString = nullptr;	//	a sequence of characters, either "2" or "R" or "C", one character for each input image. describes whether the shader was compiled to work with 2D textures or RECT textures or cube textures.

		//	access to these vars should be restricted by the '_renderLock' var inherited from GLScene
		GLCachedAttrib	_vertexAttrib = GLCachedAttrib("VERTEXDATA");	//	caches the location of the attribute in the compiled GL program for the vertex input
		GLCachedUni		_renderSizeUni = GLCachedUni("RENDERSIZE");	//	caches the location of the uniform in the compiled GL program for the render size
		GLCachedUni		_passIndexUni = GLCachedUni("PASSINDEX");	//	caches the location of the uniform in the compiled GL program for the pass index
		GLCachedUni		_timeUni = GLCachedUni("TIME");	//	caches the location of the uniform in the compiled GL program for the time
		GLCachedUni		_timeDeltaUni = GLCachedUni("TIMEDELTA");	//	caches the location of the uniform in the compiled GL program for the time delta
		GLCachedUni		_dateUni = GLCachedUni("DATE");	//	caches the location of the uniform in the compiled GL program for the date
		GLCachedUni		_renderFrameIndexUni = GLCachedUni("FRAMEINDEX");	//	caches the location of the uniform in the compiled GL program for the frame index

		//	access to these vars should be restricted by the '_renderLock' var inherited from GLScene
		//GLBufferRef			geoXYVBO = nullptr;
#if !defined(VVGL_TARGETENV_GLES)
		GLBufferRef			_vao = nullptr;
#endif
		GLBufferRef			_vbo = nullptr;
		Quad<VertXY>		_vboContents;	//	the VBO owned by VAO is described by this var- we check this, and if there's a delta then we have to upload new data to the VBO

		//	these vars describe some non-default/non-standard options for more unusual situations
		bool				_alwaysRenderToFloat = false;	//	false by default- if true, all interim buffers generated by the ISF will be float32 per component.  set this before loading the doc.
		bool				_persistentToIOSurface = false;	//	false by default- if true, persistent buffers generated by the ISF will be backed by IOSurfaces (so they can be re-used if the underlying GL context changes to one in a different sharegroup).  set this before loading the doc.
		GLBufferPoolRef		_privatePool = nullptr;	//	by default this is null and the scene will try to use the global buffer pool to create interim resources (temp/persistent buffers).  if non-null, the scene will use this pool to create interim resources.
		GLTexToTexCopierRef	_privateCopier = nullptr;	//	by default this is null and the scene will try to use the global buffer copier to create interim resources.  if non-null, the scene will use this copier to create interim resources.

	public:
		//!	Creates a new OpenGL context that shares the global buffer pool's context
		ISFScene();
		//!	Creates a new ISFScene instance, but not a new OpenGL context- instead it uses the passed GLContext.
		ISFScene(const GLContextRef & inCtx);
		virtual ~ISFScene();

		virtual void prepareToBeDeleted();
		
		
		/*!
		\name Loading ISF files
		*/
		///@{
		
		//!	Unloads whatever ISF file is currently loaded.
		void useFile() throw(ISFErr);
		//!	Loads the ISF file at the passed path.
		void useFile(const string & inPath) throw(ISFErr);
		//!	Starts using the ISF file represented by the passed ISFDoc.
		void useDoc(ISFDocRef & inDoc);
		//!	Returns the ISFDoc currently being used by the scene.  Interacting with this doc by setting the value of its inputs will directly affect rendering.
		inline ISFDocRef doc() { lock_guard<mutex> lock(_propertyLock); return _doc; }
		
		///@}
		
		
		/*!
		\name Uncommon setter/getters
		*/
		///@{
		
		//!	Sets the receiver's _alwaysRenderToFloat flag- if true, all frames will be rendered using high-bit-depth textures (usually 32 bits per channel/128 bits per pixel).  Default is false.
		void setAlwaysRenderToFloat(const bool & n) { _alwaysRenderToFloat=n; }
		//!	Gets the receiver's _alwaysRenderToFloat flag.
		bool alwaysRenderToFloat() { return _alwaysRenderToFloat; }
		//!	Sets the receiver's _persistentToIOSurface flag- if true, all passes that are flagged as persistent will render to IOSurface-backed GL textures (a mac-specific optimization that means the textures can be shared with other processes).  Defaults to false.
		void setPersistentToIOSurface(const bool & n) { _persistentToIOSurface=n; }
		//!	Gets the receiver's _persistentToIOSurface flag.
		bool persistentToIOSurface() { return _persistentToIOSurface; }
		//!	Sets the receiver's private buffer pool (which should default to null).  If non-null, this buffer pool will be used to generate any GL resources required by this scene.  Handy if you have a variety of GL contexts that aren't shared and you have to switch between them rapidly on a per-frame basis.
		void setPrivatePool(const GLBufferPoolRef & n) { _privatePool=n; }
		//!	Gets the receiver's private buffer pool- null by default, only non-null if something called setPrivatePool().
		GLBufferPoolRef privatePool() { return _privatePool; }
		//!	Sets the receiver's private buffer copier (which should default to null).  If non-null, this copier will be used to copy any resources that need to be copied- like setPrivatePool(), handy if you have a variety of GL contexts that aren't shared and you have to switch between them rapidly on a per-frame basis.
		void setPrivateCopier(const GLTexToTexCopierRef & n) { _privateCopier=n; }
		//!	Gets the receiver's private buffer copier- null by default, only non-null if something called setPrivateCopier().
		GLTexToTexCopierRef privateCopier() { return _privateCopier; }
		
		///@}
		
		
		/*!
		\name Setting/getting images and values from INPUTS and PASSES
		*/
		///@{
		
		//!	Locates the attribute/INPUT with the passed name, and sets its current value to the passed GLBuffer.
		void setBufferForInputNamed(const GLBufferRef & inBuffer, const string & inName);
		//!	Assumes that the receiver has loaded a filter-type ISF file- locates the attribute/INPUT that corresponds to the image filter input, and sets its current value to the passed GLBuffer.
		void setFilterInputBuffer(const GLBufferRef & inBuffer);
		//!	Locates the image-type input with the passed name, and sets its current value to the passed GLBuffer.
		void setBufferForInputImageKey(const GLBufferRef & inBuffer, const string & inString);
		//!	Locates the audio-type or audioFFT-type input with the passed name, and sets its current value to the passed GLBuffer.
		void setBufferForAudioInputKey(const GLBufferRef & inBuffer, const string & inString);
		//!	Locates the image-type input matching the passed string, and gets its current value (a GLBufferRef, or null).
		GLBufferRef getBufferForImageInput(const string & inKey);
		//!	Locates the audio-type input matching the passed string, and gets its current value (a GLBufferRef, or null).
		GLBufferRef getBufferForAudioInput(const string & inKey);
		//!	Locates the render pass flagged to render to a persistent buffer with a name that matches the passed string, and gets its current value (a GLBufferRef, or null).
		GLBufferRef getPersistentBufferNamed(const string & inKey);
		//!	Locates the render pass not flagged to render to a persistent buffer with a name that matches the passed string, and gets its current value (a GLBufferRef, or null).
		GLBufferRef getTempBufferNamed(const string & inKey);
		
		//!	Locates the attribute/INPUT with the passed name, and sets its current value to the passed ISFVal.  Can be used to set the value of inputs that use images to express their values, but there are dedicated functions which are easier to work with because they don't wrap the GLBufferRef up inside an ISFVal.
		void setValueForInputNamed(const ISFVal & inVal, const string & inName);
		//!	Locates the attribute/INPUT with the passed name, and gets its current value (an ISFVal).  Attributes whose values are expressed as images vend ISFVal instances which in turn vend GLBufferRef instances.
		ISFVal valueForInputNamed(const string & inName);
		
		///@}
		
		
		/*!
		\name Rendering to GLBuffer
		*/
		///@{
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverloaded-virtual"
		//virtual GLBufferRef createAndRenderABuffer(const VVGL::Size & inSize=VVGL::Size(640.,480.), const GLBufferPoolRef & inPool=GetGlobalBufferPool());
		/*!
		\brief Creates a buffer of the appropriate type (defaults to 8 bits per channel unless the ISF explicitly requires a floating point texture) and renders into it.
		\param inSize The size of the frame you want to render.
		\param outPassDict Either null or a ptr to a valid map instance of the appropriate type- the output of each render pass is stored in this map.
		\param inPoolRef The buffer pool to use to create GL resources for this frame (defaults to the global buffer pool).
		*/
		virtual GLBufferRef createAndRenderABuffer(const VVGL::Size & inSize=VVGL::Size(640.,480.), map<int32_t,GLBufferRef> * outPassDict=nullptr, const GLBufferPoolRef & inPoolRef=GetGlobalBufferPool());
		//virtual GLBufferRef createAndRenderABuffer(const VVGL::Size & inSize, const double & inRenderTime, const GLBufferPoolRef & inPoolRef=GetGlobalBufferPool());
		/*!
		\brief Creates a buffer of the appropriate type (defaults to 8 bits per channel unless the ISF explicitly requires a floating point texture) and renders into it.
		\param inSize The size of the frame you want to render.
		\param inRenderTime The explicit time to use when rendering the frame.
		\param outPassDict Either null or a ptr to a valid map instance of the appropriate type- the output of each render pass is stored in this map.
		\param inPoolRef The buffer pool to use to create GL resources for this frame (defaults to the global buffer pool).
		*/
		virtual GLBufferRef createAndRenderABuffer(const VVGL::Size & inSize, const double & inRenderTime, map<int32_t,GLBufferRef> * outPassDict=nullptr, const GLBufferPoolRef & inPoolRef=GetGlobalBufferPool());
#pragma clang diagnostic pop
		///@}
		
		
		void renderToBuffer(const GLBufferRef & inTargetBuffer, const VVGL::Size & inRenderSize, const double & inRenderTime, map<int32_t,GLBufferRef> * outPassDict);
		void renderToBuffer(const GLBufferRef & inTargetBuffer, const VVGL::Size & inRenderSize, const double & inRenderTime);
		void renderToBuffer(const GLBufferRef & inTargetBuffer, const VVGL::Size & inRenderSize, map<int32_t,GLBufferRef> * outPassDict);
		void renderToBuffer(const GLBufferRef & inTargetBuffer, const VVGL::Size & inRenderSize);
		void renderToBuffer(const GLBufferRef & inTargetBuffer);

		virtual void setSize(const VVGL::Size & n);
		VVGL::Size size() const { return _orthoSize; }
		VVGL::Size renderSize() const { return _renderSize; }
		//!	Creates a a new Timestamp relative to the scene's built-in _baseTime instance.  This is how the render time is calculated.
		inline Timestamp getTimestamp() { return Timestamp()-_baseTime; }
		//!	Sets the "throwExceptions" member var, which is false by default.  If true, the ISFScene will throw an exception using the ISFErr object to describe the nature and type of the problem.  Exceptions may be thrown if the file is missing, if there's a problem reading it or parsing the JSON, if there's a problem compiling the GLSL source code for the shaders, etc.
		inline void setThrowExceptions(const bool & n) { _throwExceptions=n; }

		//virtual void renderToBuffer(const GLBufferRef & inTargetBuffer, const VVGL::Size & inRenderSize=VVGL::Size(640.,480.), const double & inRenderTime=timestamper.nowTime().getTimeInSeconds(), map<string,GLBufferRef> * outPassDict=nullptr);
		
		
		/*!
		\name Getting attributes/INPUTS
		*/
		///!{
		
		//!	Locates and returns the attribute/INPUT matching the passed name.
		ISFAttrRef inputNamed(const string & inName);
		//!	Returns a vector of ISFAttrRef instances describing all of the attribute/INPUTS.
		vector<ISFAttrRef> inputs();
		//!	Returns a vector of ISFAttrRef instances that match the bassed ISFValType.
		vector<ISFAttrRef> inputsOfType(const ISFValType & inType);
		//!	Returns a vector of all the image-type INPUTS, represented as ISFAttr instances.
		vector<ISFAttrRef> imageInputs();
		//!	Returns a vector of all the audio- and audioFFT-type INPUTS, represented as ISFAttr instances.
		vector<ISFAttrRef> audioInputs();
		//!	Returns a vector of all the image-type IMPORTED, represented as ISFAttr instances.
		vector<ISFAttrRef> imageImports();
		
		///@}
		
		
		//!	You probably shouldn't call this method directly on this subclass.
		virtual void setVertexShaderString(const string & n);
		//!	You probably shouldn't call this method directly on this subclass.
		virtual void setFragmentShaderString(const string & n);

	protected:
#if !defined(VVGL_TARGETENV_GLES)
		inline GLBufferRef vao() const { return _vao; }
		inline void setVAO(const GLBufferRef & n) { _vao = n; }
#endif
		inline GLBufferRef vbo() const { return _vbo; }
		inline void setVBO(const GLBufferRef & n) { _vbo = n; }
		void _setUpRenderCallback();
		virtual void _renderPrep();
		virtual void _initialize();
		virtual void _renderCleanup();
		virtual void _render(const GLBufferRef & inTargetBuffer, const VVGL::Size & inSize, const double & inTime, map<int32_t,GLBufferRef> * outPassDict);

};




/*!
\relatedalso ISFScene
\brief Creates and returns an ISFScene.  The scene makes a new GL context which shares the context of the global buffer pool.
*/
inline ISFSceneRef CreateISFSceneRef() { return make_shared<ISFScene>(); }
/*!
\relatedalso ISFScene
\brief Creates and returns an ISFScene.  The scene uses the passed GL context to do its drawing.
*/
inline ISFSceneRef CreateISFSceneRefUsing(const GLContextRef & inCtx) { return make_shared<ISFScene>(inCtx); }




}




#endif /* ISFScene_hpp */
