#include "snow_options.h"
#include <opengl/opengl.h>
#include <composite/composite.h>

CompString pname="snow";

struct SnowScreen :
	public ScreenInterface,
	public CompositeScreenInterface,
	public GLScreenInterface,
	public PluginClassHandler<SnowScreen,CompScreen>,
	public SnowOptions
{
	struct SnowTexture
	{
		static SnowTexture* create(CompString &s);
		~SnowTexture();
		GLTexture* texture;
		GLuint dList;
	};
	struct SnowFlake
	{
		float x, y, z;
		float xs, ys, zs;
		float ra; /* rotation angle */
		float rs; /* rotation speed */
		void think (int ms);
		void move (int ms);
		void put ();
		void set ();
		void render();
		const SnowTexture *tex;
	};
	void update()
	{
		if(snowTex.empty())
			return;
		foreach(SnowFlake& snowFlake, snowFlakes)
			snowFlake.tex = snowTex[rand () % snowTex.size()];
	}

	SnowScreen ( CompScreen *s );
	~SnowScreen ();
	CompScreen *compScreen;
	CompositeScreen *cScreen;
	GLScreen *gScreen;
	void OptionChanged(CompOption*opt, Options num);
	bool step();
	void beginRendering ();
	bool activated;
	GLuint displayList;
	void setupDisplayList(float size);
	void updateFiles(CompOption::Value::Vector &snowTexFiles);
	std::vector<SnowTexture *> snowTex;
	sigc::connection connection;

	bool glPaintOutput(const GLScreenPaintAttrib &sa, const GLMatrix &transform,const  CompRegion &region, CompOutput *output, unsigned int mask);

	bool snowToggle(CompAction* action, CompAction::State state, CompOption::Vector &opt);
	std::vector<SnowFlake> snowFlakes;
};

class SnowWindow :
	public WindowInterface,
	public GLWindowInterface,
	public PluginClassHandler<SnowWindow,CompWindow>,
	public SnowOptions
{
public:
	SnowWindow ( CompWindow* w );
	bool glDraw(const GLMatrix& transform, const GLWindowPaintAttrib &attrib, const CompRegion &region, unsigned mask);
private:
	CompWindow *window;
	GLWindow *gWindow;
};

struct SnowVTable : public CompPlugin::VTableForScreenAndWindow<SnowScreen,SnowWindow>
{
	bool init();
};
