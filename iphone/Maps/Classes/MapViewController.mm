#import "MapViewController.hpp"
#import "EAGLView.hpp"
#import "SettingsManager.h"

#include "RenderContext.hpp"
#include "WindowHandle.hpp"
#include "../../geometry/rect2d.hpp"
#include "../../yg/internal/opengl.hpp"
#include "../../yg/screen.hpp"
#include "../../map/drawer_yg.hpp"
#include "../../map/storage.hpp"

/// Location service will stop only when received lat long will be earlier than this value
#define MAX_SECONDS_INTERVAL_FOR_RECENT_LOCATION 60.0

typedef FrameWork<model::FeaturesFetcher, Navigator, iphone::WindowHandle> framework_t;

@implementation MapViewController

  framework_t * m_framework = NULL;
  mapinfo::Storage m_storage;
  
- (void) OnMyPositionClicked: (id)sender
{
	[m_locationController Start];
}

- (void) OnSettingsClicked: (id)sender
{
	[SettingsManager Show:self WithStorage:m_storage];
}

- (void) OnShowAllClicked: (id)sender
{
	m_framework->ShowAll();
}

- (void) dealloc
{
	[m_locationController release];
	delete m_framework;
  [super dealloc];
}

- (id) initWithCoder: (NSCoder *)coder
{
	if ((self = [super initWithCoder:coder]))
	{
		[(EAGLView*)self.view setController : self];
		
		shared_ptr<iphone::WindowHandle> windowHandle = [(EAGLView*)self.view windowHandle];
		shared_ptr<yg::ResourceManager> resourceManager = [(EAGLView*)self.view resourceManager];
		m_framework = new framework_t(windowHandle);
		m_framework->Init(m_storage);
		
		m_locationController = [[UserLocationController alloc] initWithDelegate:self];
		
		m_CurrentAction = NOTHING;

		// initialize with currently active screen orientation
    [self didRotateFromInterfaceOrientation: self.interfaceOrientation];
    
		m_framework->initializeGL([(EAGLView*)self.view renderContext], resourceManager);		
		
		m_framework->ShowAll();
	}
	
	return self;
}

- (void) OnLocation: (m2::PointD const &) mercatorPoint withTimestamp: (NSDate *) timestamp
{
	// stop location update to preserve battery, but only if received location is up to date
  NSTimeInterval secondsFromLastUpdate = [timestamp timeIntervalSinceNow];
  if (fabs(secondsFromLastUpdate) < MAX_SECONDS_INTERVAL_FOR_RECENT_LOCATION)
  	[m_locationController Stop];
  m_framework->SetMyPosition(mercatorPoint);
}

- (void) OnLocationError: (NSString *) errorDescription
{
	NSLog(@"Error: %@", errorDescription);
}

- (void)onResize:(GLint) width withHeight:(GLint) height
{	
	UIInterfaceOrientation orientation = [self interfaceOrientation];
	if ((orientation == UIInterfaceOrientationLandscapeLeft)
		||(orientation == UIInterfaceOrientationLandscapeRight))
		std::swap(width, height);
	NSLog(@"onResize: %d, %d", width, height);
	m_framework->OnSize(width, height);
}

- (void)updatePointsFromEvent:(UIEvent*)event
{
	NSSet * allTouches = [event allTouches];
	int touchCount = [allTouches count];
	if (touchCount == 1)
	{
		CGPoint pt = [[[allTouches allObjects] objectAtIndex:0] locationInView:nil];
		m_Pt1 = m2::PointD(pt.x * self.view.contentScaleFactor, pt.y * self.view.contentScaleFactor);
	}
	else
	{
		CGPoint pt1 = [[[allTouches allObjects] objectAtIndex:0] locationInView:nil];
		CGPoint pt2 = [[[allTouches allObjects] objectAtIndex:1] locationInView:nil];
		m_Pt1 = m2::PointD(pt1.x * self.view.contentScaleFactor, pt1.y * self.view.contentScaleFactor);
		m_Pt2 = m2::PointD(pt2.x * self.view.contentScaleFactor, pt2.y * self.view.contentScaleFactor);
	}
}

- (void)stopCurrentAction
{
	switch (m_CurrentAction)
	{
		case NOTHING:
			break;
		case DRAGGING:
			m_framework->StopDrag(DragEvent(m_Pt1.x, m_Pt1.y));
			break;
		case SCALING:
			m_framework->StopScale(ScaleEvent(m_Pt1.x, m_Pt1.y, m_Pt2.x, m_Pt2.y));
			break;
	}
	m_CurrentAction = NOTHING;
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self updatePointsFromEvent:event];
	int touchCount = [[event allTouches] count];
	// NSLog(@"touchesBeg %i", touchCount);
	if (touchCount == 1)
	{
		m_framework->StartDrag(DragEvent(m_Pt1.x, m_Pt1.y));
		m_CurrentAction = DRAGGING;
	}
	else
	{
		m_framework->StartScale(ScaleEvent(m_Pt1.x, m_Pt1.y, m_Pt2.x, m_Pt2.y));
		m_CurrentAction = SCALING;
	}
}

- (void)touchesMoved:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self updatePointsFromEvent:event];
	
	bool needRedraw = false;
	
	switch (m_CurrentAction)
	{
	case NOTHING:
		return;
	case DRAGGING:
		m_framework->DoDrag(DragEvent(m_Pt1.x, m_Pt1.y));
		needRedraw = true;
		break;
	case SCALING:
		if ([[event allTouches] count] < 2)
			[self stopCurrentAction];
		else
		{
			m_framework->DoScale(ScaleEvent(m_Pt1.x, m_Pt1.y, m_Pt2.x, m_Pt2.y));
			needRedraw = true;
		}
		break;
	}
}

- (void)touchesEnded:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self updatePointsFromEvent:event];
	[self stopCurrentAction];
	
	if (((UITouch*)[touches anyObject]).tapCount == 2)
		m_framework->ScaleToPoint(ScaleToPointEvent(m_Pt1.x, m_Pt1.y, 2));
}

- (void)touchesCancelled:(NSSet*)touches withEvent:(UIEvent*)event
{
	[self stopCurrentAction];
}

- (void)onPaint
{
	shared_ptr<iphone::WindowHandle> windowHandle = [(EAGLView*)self.view windowHandle];
	shared_ptr<PaintEvent> paintEvent(new PaintEvent(windowHandle->drawer()));
	windowHandle->drawer()->beginFrame();
	windowHandle->drawer()->clear();
	m_framework->Paint(paintEvent);
	windowHandle->drawer()->endFrame();
}


/*
// Implement loadView to create a view hierarchy programmatically, without using a nib.
- (void)loadView {
}
*/

// Implement viewDidLoad to do additional setup after loading the view, typically from a nib.
- (void)viewDidLoad
{
  [super viewDidLoad];
}


// Override to allow orientations other than the default portrait orientation.
- (BOOL) shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation) interfaceOrientation
{
	return YES;
}

- (void)didReceiveMemoryWarning
{
  // Releases the view if it doesn't have a superview.
  [super didReceiveMemoryWarning];
	
	// Release any cached data, images, etc that aren't in use.
}

- (void)viewDidUnload
{
  // Release any retained subviews of the main view.
  // e.g. self.myOutlet = nil;
}

- (void) didRotateFromInterfaceOrientation: (UIInterfaceOrientation) fromInterfaceOrientation
{
	EOrientation newOrientation;
	switch (self.interfaceOrientation)
  {
		case UIInterfaceOrientationPortrait: newOrientation = EOrientation0;break;
		case UIInterfaceOrientationPortraitUpsideDown: newOrientation = EOrientation180; break;
		case UIInterfaceOrientationLandscapeLeft: newOrientation = EOrientation90; break;
		case UIInterfaceOrientationLandscapeRight: newOrientation = EOrientation270; break;
  }
	m_framework->SetOrientation(newOrientation);
}

@end
