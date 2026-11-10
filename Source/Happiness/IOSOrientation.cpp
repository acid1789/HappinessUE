// Keeps the game in landscape on iPad.
//
// Info.plist already allows only landscape left/right and sets UIRequiresFullScreen, but iPadOS 26 deprecated
// UIRequiresFullScreen and treats apps as resizable, so the system turned the game to portrait with the iPad. Apple's
// replacement (TN3192) is for the view controller to report prefersInterfaceOrientationLocked, which UE 5.8's
// IOSViewController doesn't, so this category adds it from the game module.
//
// The lock holds whatever presentation is current. Started with the iPad held upright, iPadOS shows the landscape
// window letterboxed in the portrait screen (scene orientation landscape, iPad upright); locking then
// would keep it letterboxed after the iPad is turned. So the lock only applies once the window fills a landscape
// screen, and it is re-evaluated whenever the iPad is rotated. If a later engine version handles this, remove this file.

#include "CoreMinimal.h"

#if PLATFORM_IOS && !PLATFORM_TVOS

#include "IOS/IOSAppDelegate.h"
#include "IOS/IOSView.h"

namespace
{
	bool IsSceneLandscape(UIWindowScene* Scene)
	{
		return Scene != nil && UIInterfaceOrientationIsLandscape(Scene.effectiveGeometry.interfaceOrientation);
	}

	// A landscape window shown inside the iPad held upright, with black bars above and below. iPadOS describes the
	// screen in the window's terms then (UIScreen bounds are landscape), so this goes by how the iPad is held.
	bool IsLetterboxed(UIWindowScene* Scene)
	{
		return IsSceneLandscape(Scene) && UIDeviceOrientationIsPortrait([UIDevice currentDevice].orientation);
	}

	bool ShouldLock(UIWindowScene* Scene)
	{
		const bool bLock = IsSceneLandscape(Scene) && !IsLetterboxed(Scene);
		const CGRect InFixed = [Scene.coordinateSpace convertRect:Scene.coordinateSpace.bounds toCoordinateSpace:Scene.screen.fixedCoordinateSpace];
		UE_LOG(LogTemp, Log, TEXT("IOSOrientation: orientation %d, device %d (1/2 upright, 3/4 sideways, 5/6 flat), window in fixed screen space %.0fx%.0f -> %s"),
			Scene != nil ? (int)Scene.effectiveGeometry.interfaceOrientation : 0, (int)[UIDevice currentDevice].orientation,
			InFixed.size.width, InFixed.size.height, bLock ? TEXT("locked") : TEXT("unlocked"));
		return bLock;
	}

	void UpdateLock(UIWindowScene* Scene)
	{
		[Scene.keyWindow.rootViewController setNeedsUpdateOfPrefersInterfaceOrientationLocked];
	}

	void UpdateLockOfActiveScene()
	{
		for (UIScene* Scene in [UIApplication sharedApplication].connectedScenes)
		{
			if ([Scene isKindOfClass:[UIWindowScene class]] && Scene.activationState == UISceneActivationStateForegroundActive)
			{
				UpdateLock((UIWindowScene*)Scene);
			}
		}
	}

	void RequestLandscape(UIWindowScene* Scene)
	{
		if (Scene == nil)
		{
			return;
		}
		if (!IsSceneLandscape(Scene))
		{
			UIWindowSceneGeometryPreferencesIOS* Preferences =
				[[UIWindowSceneGeometryPreferencesIOS alloc] initWithInterfaceOrientations:UIInterfaceOrientationMaskLandscape];
			[Scene requestGeometryUpdateWithPreferences:Preferences errorHandler:^(NSError* Error) {
				UE_LOG(LogTemp, Warning, TEXT("IOSOrientation: landscape request refused: %s"), *FString(Error.localizedDescription));
			}];
		}
		UpdateLock(Scene);
	}
}

@implementation IOSViewController (HappinessOrientationLock)

+ (void)load
{
	NSNotificationCenter* Center = [NSNotificationCenter defaultCenter];
	[Center addObserverForName:UISceneDidActivateNotification object:nil queue:[NSOperationQueue mainQueue]
		usingBlock:^(NSNotification* Note) {
			if ([Note.object isKindOfClass:[UIWindowScene class]])
			{
				RequestLandscape((UIWindowScene*)Note.object);
			}
		}];

	// Turning the iPad can end (or start) the letterboxed presentation; the geometry settles a moment after the
	// rotation is reported, so check again shortly after too
	dispatch_async(dispatch_get_main_queue(), ^{
		[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
	});
	[Center addObserverForName:UIDeviceOrientationDidChangeNotification object:nil queue:[NSOperationQueue mainQueue]
		usingBlock:^(NSNotification* Note) {
			UpdateLockOfActiveScene();
			dispatch_after(dispatch_time(DISPATCH_TIME_NOW, (int64_t)(0.5 * NSEC_PER_SEC)), dispatch_get_main_queue(), ^{
				UpdateLockOfActiveScene();
			});
		}];
}

- (BOOL)prefersInterfaceOrientationLocked
{
	return ShouldLock(self.view.window.windowScene);
}

@end

#endif
