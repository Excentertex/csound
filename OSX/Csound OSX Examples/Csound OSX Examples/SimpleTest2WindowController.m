//
//  SimpleTest2WindowController.m
//  Csound OSX Examples
//
//  Created by Aurelius Prochazka on 7/14/14.
//
//

#import "SimpleTest2WindowController.h"
#import "CsoundUI.h"

@interface SimpleTest2WindowController() {
    CsoundObj* csound;
}
@property (strong) IBOutlet NSButton *startStopButton;
@property (strong) IBOutlet NSSlider *rateSlider;
@property (strong) IBOutlet NSSlider *durationSlider;
@property (strong) IBOutlet NSSlider *attackSlider;
@property (strong) IBOutlet NSSlider *decaySlider;
@property (strong) IBOutlet NSSlider *sustainSlider;
@property (strong) IBOutlet NSSlider *releaseSlider;
@end

@implementation SimpleTest2WindowController

- (IBAction)toggleOnOff:(id)sender {
    
	if([self.startStopButton.title isEqualToString:@"Start"]) {
        
        NSString *tempFile = [[NSBundle mainBundle] pathForResource:@"test2" ofType:@"csd"];
        
        csound = [[CsoundObj alloc] init];
        [csound addCompletionListener:self];
        
        CsoundUI *csoundUI = [[CsoundUI alloc] initWithCsoundObj:csound];
        [csoundUI addSlider:_rateSlider     forChannelName:@"noteRate"];
        [csoundUI addSlider:_durationSlider forChannelName:@"duration"];
        [csoundUI addSlider:_attackSlider   forChannelName:@"attack"];
        [csoundUI addSlider:_decaySlider    forChannelName:@"decay"];
        [csoundUI addSlider:_sustainSlider  forChannelName:@"sustain"];
        [csoundUI addSlider:_releaseSlider  forChannelName:@"release"];
        
        [csound startCsound:tempFile];
        
	} else {
        NSLog(@"try to stop csound");
        [csound stopCsound];
    }
}

#pragma mark CsoundObjCompletionListener

-(void)csoundObjDidStart:(CsoundObj *)csoundObj {
    self.startStopButton.title = @"Stop";
}

-(void)csoundObjComplete:(CsoundObj *)csoundObj {
	self.startStopButton.title = @"Start";
}

@end
