//vst4cs --- VST host opcodes for Csound5
//By: Andres Cabrera
//June 2004
//Using code by Hermann Seib and from the vst~ object in pd (which in turn takes from psycle tracker

#include "AEffEditor.hpp"
#include "aeffectx.h"
#include "vsthost.h"

VstTimeInfo VSTPlugin::_timeInfo;

float VSTPlugin::sample_rate = 44100;

VSTPlugin::VSTPlugin(ENVIRON *csound_) : 
    w(0), 
    csound(csound_), 
    aeffect(0), 
    libraryHandle(0), 
    queue_size(0), 
    instantiated(false), 
    overwrite(false),
    show_params(false),
    _midichannel(0),
    edited(false),
    blockSize(0)
{
    vstEvents = (VstEvents *)&buffer[0];
	 //w = GetForegroundWindow();
}

VSTPlugin::~VSTPlugin()
{
	Free();			
}

bool VSTPlugin::AddMIDI(int data0, int data1, int data2)
{
	if (instantiated) {
		VstMidiEvent* pevent=&midievent[queue_size];
		pevent->type = kVstMidiType;
		pevent->byteSize = 24;
		pevent->deltaFrames = 0;
		pevent->flags = 0;
		pevent->detune = 0;
		pevent->noteLength = 0;
		pevent->noteOffset = 0;
		pevent->reserved1 = 0;
		pevent->reserved2 = 0;
		pevent->noteOffVelocity = 0;
		pevent->midiData[0] = data0;
		pevent->midiData[1] = data1;
		pevent->midiData[2] = data2;
		pevent->midiData[3] = 0;
		if ( queue_size < MAX_EVENTS ) 
            queue_size++;
		SendMidi();
		return true;
	}
	else 
        return false;
}

bool VSTPlugin::AddNoteOn( int note, int speed, int midichannel)
{
	if (instantiated) {
		VstMidiEvent* pevent=&midievent[queue_size];
		pevent->type = kVstMidiType;
		pevent->byteSize = 24;
		pevent->deltaFrames = 0;
		pevent->flags = 0;
		pevent->detune = 0;
		pevent->noteLength = 0;
		pevent->noteOffset = 0;
		pevent->reserved1 = 0;
		pevent->reserved2 = 0;
		pevent->noteOffVelocity = 0;
		pevent->midiData[0] = 144 | midichannel;
		pevent->midiData[1] = note;
		pevent->midiData[2] = speed;
		pevent->midiData[3] = 0;
		if ( queue_size < MAX_EVENTS ) 
            queue_size++;
		SendMidi();
		return true;
	}
	else 
        return false;
}

bool VSTPlugin::AddNoteOff( int note, int midichannel)
{
	if (instantiated) {
		VstMidiEvent* pevent=&midievent[queue_size];
		pevent->type = kVstMidiType;
		pevent->byteSize = 24;
		pevent->deltaFrames = 0;
		pevent->flags = 0;
		pevent->detune = 0;
		pevent->noteLength = 0;
		pevent->noteOffset = 0;
		pevent->reserved1 = 0;
		pevent->reserved2 = 0;
		pevent->noteOffVelocity = 0;
		pevent->midiData[0] = 128 | midichannel;
		pevent->midiData[1] = note;
		pevent->midiData[2] = 0;
		pevent->midiData[3] = 0;
		if ( queue_size < MAX_EVENTS ) 
            queue_size++;
		SendMidi();
		return true;
	}
	else 
        return false;
}

bool VSTPlugin::DescribeValue(int p,char* value)
{
	int parameter = p;
	if(instantiated)
	{
		if(parameter<aeffect->numParams)
		{
//			char par_name[64];
			char par_display[64];
			char par_label[64];
//			Dispatch(effGetParamName,parameter,0,par_name,0.0f);
			Dispatch(effGetParamDisplay,parameter,0,par_display,0.0f);
			Dispatch(effGetParamLabel,parameter,0,par_label,0.0f);
//			sprintf(psTxt,"%s:%s%s",par_name,par_display,par_label);
			//csound->Message(csound,"%s:%s",par_display,par_label);
			*value = *par_display;
			return true;
		}
		//else	sprintf(psTxt,"NumParams Exeeded");
	}
	//else		sprintf(psTxt,"Not loaded");
	return false;
}

int VSTPlugin::Instance(const char *libraryName_)
{
	//csound->Message(csound,"Instance \n");
 	libraryHandle = csound->OpenLibrary(libraryName_);
	if(!libraryHandle)	
	{
		csound->Message(csound,"WARNING! '%s' was not found or is invalid.\n", libraryName_);
		return VSTINSTANCE_ERR_NO_VALID_FILE;
	}
	csound->Message(csound,"Loaded plugin library '%s'.\n" , libraryName_);
	PVSTMAIN main = (PVSTMAIN)csound->GetLibrarySymbol(libraryHandle,"main");
	if(!main)
	{	
	    csound->Message(csound,"Failed to find 'main' function.\n");
     	csound->CloseLibrary(libraryHandle);
		aeffect=NULL;
		instantiated=false;
		return VSTINSTANCE_ERR_NO_VST_PLUGIN;
	}
	csound->Message(csound,"Found 'main' function at 0x%x.\n" , main);
	aeffect = main((audioMasterCallback) VSTPlugin::Master);
	aeffect->user = this;
	if(!aeffect)
	{
		csound->Message(csound,"VST plugin: unable to create effect.\n");
		csound->CloseLibrary(libraryHandle);
		aeffect=NULL;
		instantiated=false;
		return VSTINSTANCE_ERR_REJECTED;
	}
	csound->Message(csound,"Created effect '%x'.\n" , aeffect);	
	if(  aeffect->magic!=kEffectMagic)
	{
		csound->Message(csound,"VST plugin: Instance query rejected by 0x%x\n",(int)aeffect);
		csound->CloseLibrary(libraryHandle);
		aeffect=NULL;
		instantiated=false;
		return VSTINSTANCE_ERR_REJECTED;
	}
	instantiated=true;
	return VSTINSTANCE_NO_ERROR;
}

void VSTPlugin::Info ()
{
	int i =0;
	csound->Message(csound,"--------------------------------------------\n");
	//csound->Message(csound,"VST plugin : Instanced at (Effect*): %.8X\n",(int)aeffect);
	if (!Dispatch( effGetProductString, 0, 0, &productName, 0.0f))
	{
		strcpy(productName, libraryName);
	}
	csound->Message(csound,"Loaded plugin: %s\n", productName);
	if (!aeffect->dispatcher(aeffect, effGetVendorString, 0, 0, &vendorName, 0.0f))
	{
		strcpy(vendorName, "Unknown vendor");
	}
	csound->Message(csound,"Vendor name:%s \n", vendorName);
	_version = aeffect->version;
	csound->Message(csound,"Version: %d \n",_version);
	_isSynth = (aeffect->flags & effFlagsIsSynth)?true:false;
	csound->Message(csound,"Is synthesizer? %s\n",(_isSynth==true?"Yes":"No"));
	overwrite = (aeffect->flags & effFlagsCanReplacing)?true:false;
	editor = (aeffect->flags & effFlagsHasEditor)?true:false;
	csound->Message(csound,"Number of inputs: %i\n", getNumInputs());
	csound->Message(csound,"Number of outputs: %i\n", getNumOutputs());
	long nparams=NumParameters();
	csound->Message(csound,"Number of parameters: %d\n",nparams);
    char buffer[256];
	for (i=0; i<nparams;i++) {
	    strcpy(buffer, "No parameter");
		GetParamName (i, buffer);
		csound->Message(csound,"  Parameter%5i: %s\n",i, buffer);		
	}
	csound->Message(csound,"Number of programs: %d\n",aeffect->numPrograms);
	for (i = 0; i < aeffect->numPrograms; i++) {
        strcpy(buffer, "No program");
	    GetProgramName(0, i, buffer);
	    csound->Message(csound, "  Program%7i: %s\n", i, buffer);
    }	
}

int VSTPlugin::getNumInputs( void )
{
	return aeffect->numInputs;
}

int VSTPlugin::getNumOutputs( void )
{
	return aeffect->numOutputs;
}

void VSTPlugin::Free() // Called also in destruction
{
	if(instantiated)
	{
		instantiated=false;
		//wxLogMessage(_T("VST plugin : Free query 0x%.8X\n"),(int)aeffect);
		aeffect->user = NULL;
		Dispatch( effMainsChanged, 0, 0, NULL, 0.0f);
		Dispatch( effClose,        0, 0, NULL, 0.0f);
//		delete aeffect; // <-  Should check for the necessity of this command.
		aeffect=NULL;
		csound->CloseLibrary(libraryHandle);
	}
}

void VSTPlugin::Init( float samplerate , float blocksize )
{
	sample_rate = samplerate;
	blockSize = blocksize;
	//csound->Message(csound,"init ok\n");
	Dispatch(effOpen        ,  0, 0, NULL, 0.f);
	Dispatch(effSetProgram  ,  0, 0, NULL, 0.0f);
	Dispatch(effMainsChanged,  0, 1, NULL, 0.f);
	Dispatch(effSetSampleRate, 0, 0, 0, (float) sample_rate );
	Dispatch(effSetBlockSize,  0, blocksize, NULL, 0.f );
	size_t framesPerBlock = csound->GetKsmps(csound);
	size_t channels = csound->GetNchnls(csound);
	inputs = new float *[channels];
  	outputs = new float *[channels];
  	for(size_t i = 0; i < channels; i++) {
  	   inputs[i] = new float[framesPerBlock];
  	   outputs[i] = new float[framesPerBlock];
	}
}


bool VSTPlugin::SetParameter(int parameter, float value)
{
	if(instantiated) {
		if (( parameter >= 0 ) && (parameter<=aeffect->numParams)) {
			aeffect->setParameter(aeffect,parameter,value);
			return true;
		}
		else return false;
	}
	return false;
}

bool VSTPlugin::SetParameter(int parameter, int value)
{
	return SetParameter(parameter,value/65535.0f);
}

int VSTPlugin::GetCurrentProgram()
{
	if(instantiated)
		return Dispatch(effGetProgram,0,0,NULL,0.0f);
	else
		return 0;
}

void VSTPlugin::SetCurrentProgram(int prg)
{
	if(instantiated)
		Dispatch(effSetProgram,0,prg,NULL,0.0f);
}


void VSTPlugin::SendMidi()
{
	if(instantiated && queue_size>0) {
		vstEvents->numEvents = queue_size;
		vstEvents->reserved  = 0;
		for(int q=0;q<queue_size;q++) {
		    VstMidiEvent &vstMidiEvent = midievent[q];
		    VstEvent &vstEvent = (VstEvent &)vstMidiEvent;
            vstEvents->events[q] = &vstEvent;
            csound->Message(csound, 
                "VSTPlugin::SendMidi(status %d data1 %d data2 %d\n",
                vstMidiEvent.midiData[0],
                vstMidiEvent.midiData[1],
                vstMidiEvent.midiData[2]);
        }           
		Dispatch(effProcessEvents, 0, 0, vstEvents, 0.0f);
		queue_size=0;
	}
}

void VSTPlugin::processReplacing( float **inputs, float **outputs, long sampleframes )
{
	aeffect->processReplacing( aeffect , inputs , outputs , sampleframes );
}

void VSTPlugin::process( float **inputs, float **outputs, long sampleframes )
{
	aeffect->process( aeffect , inputs , outputs , sampleframes );
}

long VSTPlugin::Master(AEffect *effect, long opcode, long index, long value, void *ptr, float opt)
{
    //fprintf(stdout, "VSTPlugin::Master(AEffect 0x%x, opcode %d, index %d, "
    //    "value %d, ptr 0x%x, opt %f)\n",
    //    effect, opcode, index, value, ptr, opt);
    VSTPlugin *plugin = 0;
    ENVIRON *csound = 0;
    if(effect) {
        plugin = (VSTPlugin *)effect->user;
        if(plugin) {
            csound = plugin->csound;
        }
    }
	switch(opcode)
	{
	case audioMasterAutomate:			
		return true;		// index, value, returns 0
		
	case audioMasterVersion:			
		return OnGetVersion(effect);		// vst version, currently 7 (0 for older)
		
	case audioMasterCurrentId:			
		return vsthandle;	// returns the unique id of a plug that's currently loading
	case audioMasterIdle:
		effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0.0f);
		return 0;		// call application idle routine (this will call effEditIdle for all open editors too) 
	case audioMasterPinConnected:	
		    return !((value) ? 
        OnOutputConnected(effect, index) :
        OnInputConnected(effect, index));	// inquire if an input or output is beeing connected;
	case audioMasterWantMidi:			
		return false;
	case audioMasterProcessEvents:		
		return false; 	// Support of vst events to host is not available
	case audioMasterGetTime:
		memset(&_timeInfo, 0, sizeof(_timeInfo));
		_timeInfo.samplePos = 0;
		_timeInfo.sampleRate = sample_rate;
		return (long)&_timeInfo;
		/* TODO (#1#): Sera este el problema? */
		
	case audioMasterTempoAt:			
		return 0;

	case audioMasterNeedIdle:	
		//effect->dispatcher(effect, effIdle, 0, 0, NULL, 0.0f);  //linea del problema
		return false;

	case audioMasterGetSampleRate:		
		return sample_rate;	

	case audioMasterGetVendorString:	// Just fooling version string
		//strcpy((char*)ptr,"Steinberg");
		return 0;
	
	case audioMasterGetVendorVersion:	
		return 5000;	// HOST version 5000

	case audioMasterGetProductString:	// Just fooling product string
		//strcpy((char*)ptr,"Cubase 5.0");
		return 0;

	case audioMasterVendorSpecific:		
		{
			return 0;
		}
	case audioMasterGetLanguage:		
		return kVstLangEnglish;

	case audioMasterUpdateDisplay:
		//csound->Message(csound,"audioMasterUpdateDisplay\n");
		//effect->dispatcher(effect, effEditIdle, 0, 0, NULL, 0.0f);
		return 0;

	case 	audioMasterSetTime:						csound->Message(csound,"VST master dispatcher: Set Time\n");break;
	case 	audioMasterGetNumAutomatableParameters:	csound->Message(csound,"VST master dispatcher: GetNumAutPar\n");break;
	case 	audioMasterGetParameterQuantization:	csound->Message(csound,"VST master dispatcher: ParamQuant\n");break;
	case 	audioMasterIOChanged:					csound->Message(csound,"VST master dispatcher: IOchanged\n");break;
	case 	audioMasterSizeWindow:					csound->Message(csound,"VST master dispatcher: Size Window\n");break;
	case 	audioMasterGetBlockSize:				csound->Message(csound,"VST master dispatcher: GetBlockSize\n");break;
	case 	audioMasterGetInputLatency:				csound->Message(csound,"VST master dispatcher: GetInLatency\n");break;
	case 	audioMasterGetOutputLatency:			csound->Message(csound,"VST master dispatcher: GetOutLatency\n");break;
	case 	audioMasterGetPreviousPlug:				csound->Message(csound,"VST master dispatcher: PrevPlug\n");break;
	case 	audioMasterGetNextPlug:					csound->Message(csound,"VST master dispatcher: NextPlug\n"); return 1;break;
	case 	audioMasterWillReplaceOrAccumulate:		csound->Message(csound,"VST master dispatcher: WillReplace\n");return 1; break;
	case 	audioMasterGetCurrentProcessLevel:		return 0; break;
	case 	audioMasterGetAutomationState:			csound->Message(csound,"VST master dispatcher: GetAutState\n");
 			break;
	case 	audioMasterOfflineStart:				csound->Message(csound,"VST master dispatcher: Offlinestart\n");break;
	case 	audioMasterOfflineRead:					csound->Message(csound,"VST master dispatcher: Offlineread\n");break;
	case 	audioMasterOfflineWrite:				csound->Message(csound,"VST master dispatcher: Offlinewrite\n");break;
	case 	audioMasterOfflineGetCurrentPass:		csound->Message(csound,"VST master dispatcher: OfflineGetcurrentpass\n");break;
	case 	audioMasterOfflineGetCurrentMetaPass:	csound->Message(csound,"VST master dispatcher: GetGetCurrentMetapass\n");break;
	case 	audioMasterSetOutputSampleRate:			csound->Message(csound,"VST master dispatcher: Setsamplerate\n");break;
	case 	audioMasterGetSpeakerArrangement:		csound->Message(csound,"VST master dispatcher: Getspeaker\n");break;
	case 	audioMasterSetIcon:						csound->Message(csound,"VST master dispatcher: seticon\n");break;
	case 	audioMasterCanDo:						//csound->Message(csound,"VST master dispatcher: Can Do\n");
													OnCanDo((char *)ptr);
													//(char*)ptr = hostCanDos; /*csound->Message(csound,"%s",(char*)ptr[1])*/; 
													
         											break;
	case 	audioMasterOpenWindow:					csound->Message(csound,"VST master dispatcher: OpenWindow\n");break;
	case 	audioMasterCloseWindow:					csound->Message(csound,"VST master dispatcher: CloseWindow\n");break;
	case 	audioMasterGetDirectory:				csound->Message(csound,"VST master dispatcher: GetDirectory\n");break;
	//case	audioMasterUpdateDisplay:				csound->Message(csound,"VST master dispatcher: audioMasterUpdateDisplay");break;

	default: csound->Message(csound,"VST master dispatcher: undefined opcode: %d , %d\n",opcode , effKeysRequired )	;break;
	}	
	return 0;
}


bool VSTPlugin::replace()
{
	return overwrite;
}

void VSTPlugin::edit()
{	
	if(instantiated)
	{ 		
		if ( ( editor ) && (!edited))
		{			
			edited = true;
			//b = (CEditorThread*) AfxBeginThread(RUNTIME_CLASS( CEditorThread ) );
			/*b =  new CEditorThread();			
			b->SetPlugin( this );			
			b->CreateThread();	*/		
		}
	}
}

void VSTPlugin::EditorIdle()
{
	Dispatch(effEditIdle,0,0, w,0.0f);			
}

/*RECT VSTPlugin::GetEditorRect()
{
	RECT ret;
	ERect *r;
	Dispatch(effEditGetRect,0,0, &r,0.0f);				
	ret.top = r->top;
	ret.bottom = r->bottom;
	ret.left = r->left;
	ret.right = r->right;
	return ret;
}*/

void VSTPlugin::SetEditWindow(void *h)
{
	w = h;	
	Dispatch(effEditOpen,0,0, w,0.0f);							
}

void VSTPlugin::OnEditorCLose()
{
	Dispatch(effEditClose,0,0, w,0.0f);					
}

void VSTPlugin::SetShowParameters(bool s)
{
	show_params = s;
}

bool VSTPlugin::ShowParams()
{
	return show_params;
}

void VSTPlugin::AddAftertouch(int value)
{
	if (value < 0) value = 0; else if (value > 127) value = 127;
 	AddMIDI( (char)MIDI_NOTEOFF | _midichannel , value );
}

void VSTPlugin::AddPitchBend(int value)
{
		AddMIDI( MIDI_PITCHBEND + (_midichannel & 0xf) , ((value>>7) & 127), (value & 127));
}

void VSTPlugin::AddProgramChange(int value)
{
 if (value < 0) value = 0; else if (value > 127) value = 127;
    AddMIDI( MIDI_PROGRAMCHANGE + (_midichannel & 0xf), value, 0);
}

void VSTPlugin::AddControlChange(int control, int value)
{
  if (control < 0) control = 0;  else if (control > 127) control = 127;
    if (value < 0) value = 0;  else if (value > 127) value = 127;
    AddMIDI( MIDI_CONTROLCHANGE + (_midichannel & 0xf), control, value);
}

bool VSTPlugin::GetProgramName( int cat , int p, char *buf)
{
	int parameter = p;
	if(instantiated)
	{
		if(parameter<NumPrograms())
		{
			Dispatch(effGetProgramNameIndexed,parameter,cat,buf,0.0f);
			return true;
		}
	}
	return false;
}

int VSTPlugin::GetNumCategories()
{
	if(instantiated)
		return Dispatch(effGetNumProgramCategories,0,0,NULL,0.0f);
	else
		return 0;
}

void VSTPlugin::StopEditing()
{
	edited = false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////
////Auxiliares
////////////////////////////////////////////////////////////////////////////////////////////////////////////
long VSTPlugin::OnGetVersion(AEffect *effect)
{
#if defined(VST_2_3_EXTENSIONS)
return 2300L;
#elif defined(VST_2_2_EXTENSIONS)
return 2200L;
#elif defined(VST_2_1_EXTENSIONS)
return 2100L;
#else 
return 2L;
#endif
}

