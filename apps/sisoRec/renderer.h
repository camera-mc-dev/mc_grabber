#ifdef USE_SISO
#ifndef SISO_REC_RENDERER
#define SISO_REC_RENDERER

#include "grab.h"
#include "controls.h"

#include "renderer2/basicRenderer.h"
#include "renderer2/geomTools.h"

class RecRenderer : public Rendering::BasicRenderer
{
public:
	friend class Rendering::RendererFactory;		

	bool Step( bool &buffRecord, bool &liveRecord );
	
	void Prep( SiSoGrabber *grabber );
	
	void Update( std::vector< cv::Mat > &bgrImgs, std::map<int, bool> &dispCams );
	
	void RaiseFPSWarning()
	{
		Get2dFgRoot()->AddChild( fpsWarnNode );
	}
	void StopFPSWarning()
	{
		fpsWarnNode->RemoveFromParent();
	}
	
	// add a node as a child to one of the image cards.
	void AddImgChild( unsigned img, std::shared_ptr<Rendering::MeshNode> node );
	
	void ClearImgChildren( unsigned img );
	
protected:
	RecRenderer(unsigned width, unsigned height, std::string title);
	
	std::vector< std::shared_ptr<Rendering::MeshNode> > imgCards;
	std::shared_ptr< Rendering::SceneNode > fpsWarnNode;
	long int imgCols, imgRows;
	
	
};


void PrepRenderWindow(SiSoGrabber *grabber, unsigned &width, unsigned &height);




#endif
#endif
