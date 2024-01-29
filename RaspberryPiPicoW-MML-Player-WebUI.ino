#include <hardware/pwm.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <WebServer.h>

#define SSID_TEXT "SSID"
#define PASS_TEXT "password"


class TimeChecker {
public:
  TimeChecker() {
    start_time = 0;
    elapsed_time = 0;
  }
  
  void start() {
    start_time = micros();
  }
  
  unsigned long stop() {
    elapsed_time = micros() - start_time;
    return elapsed_time;
  }
  
private:
  unsigned long start_time;
  unsigned long elapsed_time;
};
TimeChecker timeChecker;

#define SAMPLE        64
#define MAX_AMPLITUDE 64
#define WAVEFORMS     5
#define SINE_WAVE     0
#define SQUARE_WAVE   1
#define TRIANGLE_WAVE 2
#define SAWTOOTH_WAVE 3
#define NOISE_WAVE    4
int16_t waveForm[WAVEFORMS][SAMPLE];
void initializeWaveform(){
  
  for(int i=0;i < WAVEFORMS;i++){
    for(int j=0;j < SAMPLE; j++){
      if(i == SINE_WAVE){
        waveForm[i][j] = MAX_AMPLITUDE * sin(2 * PI * j / SAMPLE);
      //0,6,12,18,24,30,35,40,45,49,53,56,59,61,62,63,64,63,62,61,59,56,53,49,45,40,35,30,24,18,12,6,0,-6,-12,-18,-24,-30,-35,-40,-45,-49,-53,-56,-59,-61,-62,-63,-64,-63,-62,-61,-59,-56,-53,-49,-45,-40,-35,-30,-24,-18,-12,-6
      } else if(i == SQUARE_WAVE){
        waveForm[i][j] = (j < SAMPLE / 2) ? MAX_AMPLITUDE : MAX_AMPLITUDE * -1;
        //64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64,-64
      } else if(i == TRIANGLE_WAVE){
        const int divider = 4;
        if(j < SAMPLE / 2){
          waveForm[i][j] = (j < SAMPLE / divider) ? j * (MAX_AMPLITUDE / (SAMPLE / divider)) : MAX_AMPLITUDE - (j - (SAMPLE / divider)) * (MAX_AMPLITUDE / (SAMPLE / divider));
        }else{
          waveForm[i][j] = (j < SAMPLE / divider * 3) ? (j - (SAMPLE / divider * 2)) * (MAX_AMPLITUDE / (SAMPLE / divider)) * -1 : (MAX_AMPLITUDE - (j - (SAMPLE / divider * 3)) * (MAX_AMPLITUDE / (SAMPLE / divider))) * -1;
        }
        //0,4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64,60,56,52,48,44,40,36,32,28,24,20,16,12,8,4,0,-4,-8,-12,-16,-20,-24,-28,-32,-36,-40,-44,-48,-52,-56,-60,-64,-60,-56,-52,-48,-44,-40,-36,-32,-28,-24,-20,-16,-12,-8,-4
      }else if(i == SAWTOOTH_WAVE){
        waveForm[i][j] = (j < (SAMPLE / 2)) ? (MAX_AMPLITUDE - (j * (MAX_AMPLITUDE / (SAMPLE / 2)))) * -1 : (j - (SAMPLE / 2)) * (MAX_AMPLITUDE / (SAMPLE / 2));
        //-64,-62,-60,-58,-56,-54,-52,-50,-48,-46,-44,-42,-40,-38,-36,-34,-32,-30,-28,-26,-24,-22,-20,-18,-16,-14,-12,-10,-8,-6,-4,-2,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62
      }else if(i == NOISE_WAVE){
        waveForm[i][j] = MAX_AMPLITUDE * sin(2 * PI * j / SAMPLE);//random((MAX_AMPLITUDE * -1),(MAX_AMPLITUDE + 1));
        //16,59,25,-32,-49,41,21,14,-29,-18,-64,22,50,10,-31,-23,-10,-37,-14,-26,51,-29,58,16,-24,-15,26,38,-7,-60,-4,61,57,59,-19,58,10,-14,9,-23,-64,11,41,-16,-30,-1,-4,-57,33,17,40,-55,50,-3,47,-40,20,13,20,12,-40,-42,28,-2
      }
    }
  }
  /*
  for (int i = 0; i < SAMPLE; i++) {
    Serial.print(String(waveForm[NOISE_WAVE][i]) + ",");
  }
  Serial.println("finish");
  while(true){delay(100);}
  */
}

#define VOLUME_MAX 100
int16_t calculateAmplitudeTable[MAX_AMPLITUDE * 2 + 1][VOLUME_MAX + 1];
void initializeCalculateAmplitudeTable(){
  for(int i=0;i<MAX_AMPLITUDE * 2 + 1;i++){
    if(i < MAX_AMPLITUDE){
      for(int j=0;j <= VOLUME_MAX;j++){
        calculateAmplitudeTable[i][j] = -((MAX_AMPLITUDE - i) * (j / 100.0));
      }
    }else if(i > MAX_AMPLITUDE){
      for(int k=0;k <= VOLUME_MAX;k++){
        calculateAmplitudeTable[i][k] = (i - MAX_AMPLITUDE) * (k / 100.0);
      }
    }
  }
  /*
  for(int i=0;i<MAX_AMPLITUDE * 2 + 1;i++){
    for(int j=0;j<VOLUME_MAX + 1;j++){
      Serial.print(calculateAmplitudeTable[i][j]);
      Serial.print(",");
    }
    Serial.println("");
  }
  */
}

#define NOTES                   88
#define FREQUENCY_CORRECTION_C4 0.05
uint16_t frequencyData[NOTES];
void initializeFrequency(){
  // A0(27.500) to C8(4186.009)
  for (int i = 0; i < NOTES; i++) {
    frequencyData[i] = 27.5 * pow(2.0, (double)i / (12.0 - FREQUENCY_CORRECTION_C4)) + 0.5;
    //frequencyData[i] = 27.5 * pow(2.0, (double)i / 12.0) + 0.5;
    //28,29,31,33,35,37,39,41,44,46,49,52,55,58,62,65,69,73,78,82,87,92,98,104,110,117,123,131,139,147,156,165,175,185,196,208,220,233,247,262,277,294,311,330,349,370,392,415,440,466,494,523,554,587,622,659,698,740,784,831,880,932,988,1047,1109,1175,1245,1319,1397,1480,1568,1661,1760,1865,1976,2093,2217,2349,2489,2637,2794,2960,3136,3322,3520,3729,3951,4186
    //0 , 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15,16,17,18,19,20,21,22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
  }
  
  /*
  for (int i = 0; i < NOTES; i++) {
    Serial.print(String(frequencyData[i]) + ",");
  }
  Serial.println("");
  */
}

#define CHANNELS  12
File mmlFile[CHANNELS];
void initializeMmlFile(){
  String temp;
  String number;
  String filename;
  
  for(int i=0;i < CHANNELS;i++){
    temp = "0" + String(i+1);
    number = temp.substring(temp.length() - 2);
    filename = "track" + number + ".mml";
    
    mmlFile[i] = LittleFS.open(filename, "r");
    
    /*
    if(mmlFile[i]){
      Serial.println(filename + ":" + String(mmlFile[i].available()));
    }else{
      Serial.println(filename + ":Failed to open file");
    }
    */
    
  }
}

#define MML_BUFFER_SIZE 200
struct TrackData{
  bool enable;
  char mml_buffer[MML_BUFFER_SIZE];
  uint16_t mml_index;
  uint16_t mml_count;
  bool enable_envelope;
  uint16_t waveform_type;
  bool enable_modulator;
  uint16_t type_modulator;
  uint16_t waveform_type_modulator;
  uint16_t octave;
  uint16_t frequency;
  uint16_t frequency_modulator;
  float frequency_ratio;
  uint32_t ticks;
  uint32_t tick;
  uint16_t base_volume;
  uint16_t volume;
  uint16_t base_volume_modulator;
  uint16_t volume_modulator;
  uint16_t status_adsr;
  uint16_t ticks_attack;
  uint16_t ticks_decay;
  uint16_t volume_sustain;
  uint16_t ticks_release;
  bool enable_envelope_modulator;
  uint16_t status_adsr_modulator;
  uint16_t ticks_attack_modulator;
  uint16_t ticks_decay_modulator;
  uint16_t volume_sustain_modulator;
  uint16_t ticks_release_modulator;
};
TrackData trackData[CHANNELS];

void initializeMmlBuffer(){
  char c;
  
  for(int i=0;i < CHANNELS;i++){
    for(int j=0;j < MML_BUFFER_SIZE;j++){
      trackData[i].mml_buffer[j] = '\0';
      if(mmlFile[i].available()){
        c = mmlFile[i].read();
        trackData[i].mml_count--;
        
        if(c == ';'){
          while(mmlFile[i].available()){
            c = mmlFile[i].read();
            trackData[i].mml_count--;
            
            if(c == '\n'){
              if(mmlFile[i].available() == false){
                break;
              }
              c = mmlFile[i].read();
              trackData[i].mml_count--;
              
              if(c == '\r'){
                if(mmlFile[i].available() == false){
                  break;
                }
                c = mmlFile[i].read();
                trackData[i].mml_count--;
              }
              
              if(c != ';'){
                trackData[i].mml_buffer[j] = c;
                break;
              }
            }
          }
        }else{
          trackData[i].mml_buffer[j] = c;
        }        
      }
    }
  }
  /*
  for(int i=0;i < CHANNELS;i++){
    Serial.println("track_" + String(i));
    for(int j=0;j < MML_BUFFER_SIZE;j++){
      int c = trackData[i].mml_buffer[j];
      //Serial.print("(");
      //Serial.print(c);
      //Serial.print(")");
      Serial.print(trackData[i].mml_buffer[j]);
    }
    Serial.println(":END");
  }
  */
  
}

#define TIMBRES 100
struct TimbreData{
  String description;
  uint16_t waveform_type;
  bool enable_envelope;
  uint16_t ticks_attack;
  uint16_t ticks_decay;
  uint16_t volume_sustain;
  uint16_t ticks_release;
  bool enable_modulator;
  uint16_t type_modulator;
  uint16_t waveform_type_modulator;
  uint16_t base_volume_modulator;
  float frequency_ratio;
  bool enable_envelope_modulator;
  uint16_t ticks_attack_modulator;
  uint16_t ticks_decay_modulator;
  uint16_t volume_sustain_modulator;
  uint16_t ticks_release_modulator;
};
TimbreData timbreData[TIMBRES];

#define DEFAULT_TICKS_ATTACK          50
#define DEFAULT_TICKS_DECAY           50
#define DEFAULT_VOLUME_SUSTAIN        50
#define DEFAULT_TICKS_RELEASE         50
#define DEFAULT_MODULATION_TYPE       0
#define DEFAULT_BASE_VOLUME_MODULATOR 100
#define DEFAULT_FREQUENCY_RATIO       1
void initializeTimbreData(){
  String temp;
  String number;
  String filename;
  
  for(int i=0;i < TIMBRES;i++){
    timbreData[i].waveform_type = SINE_WAVE;
    timbreData[i].enable_envelope = true;
    timbreData[i].ticks_attack = DEFAULT_TICKS_ATTACK;
    timbreData[i].ticks_decay = DEFAULT_TICKS_DECAY;
    timbreData[i].volume_sustain = DEFAULT_VOLUME_SUSTAIN;
    timbreData[i].ticks_release = DEFAULT_TICKS_RELEASE;
    timbreData[i].enable_modulator = false;
    timbreData[i].type_modulator = DEFAULT_MODULATION_TYPE;
    timbreData[i].waveform_type_modulator = SINE_WAVE;
    timbreData[i].base_volume_modulator = DEFAULT_BASE_VOLUME_MODULATOR;
    timbreData[i].frequency_ratio = DEFAULT_FREQUENCY_RATIO;
    timbreData[i].enable_envelope_modulator = false;
    timbreData[i].ticks_attack_modulator = DEFAULT_TICKS_ATTACK;
    timbreData[i].ticks_decay_modulator = DEFAULT_TICKS_DECAY;
    timbreData[i].volume_sustain_modulator = DEFAULT_VOLUME_SUSTAIN;
    timbreData[i].ticks_release_modulator = DEFAULT_TICKS_RELEASE;
    
    temp = "0" + String(i);
    number = temp.substring(temp.length() - 2);
    filename = "timbre" + number + ".dat";
    
    File file = LittleFS.open(filename, "r");
    
    if(file){
      //Serial.println(filename + ":" + String(file.available()));
      
      while(file.available()){
        String line = file.readStringUntil('\n');
        line.trim();
        
        String key;
        String value;
        int separator_index = line.indexOf('=');
        if(separator_index != -1){
          key = line.substring(0 ,separator_index);
          value = line.substring(separator_index + 1);
        }
        
        if(key == "WaveformType"){
          int type = value.toInt();
          if(0 <= type && type < WAVEFORMS){
            timbreData[i].waveform_type = type;
          }
        }else if(key == "EnableEnvelope"){
          int envelope = value.toInt();
          if(envelope == 0){
            timbreData[i].enable_envelope = false;
          }
        }else if(key == "TicksAttack"){
          int attack = value.toInt();
          if(attack != 0){
            timbreData[i].ticks_attack = attack;
          }
        }else if(key == "TicksDecay"){
          int decay = value.toInt();
          if(decay != 0){
            timbreData[i].ticks_decay = decay;
          }
        }else if(key == "VolumeSustain"){
          timbreData[i].volume_sustain = value.toInt();
        }else if(key == "TicksRelease"){
          int release_ = value.toInt();
          if(release_ != 0){
            timbreData[i].ticks_release = release_;
          }
        }else if(key == "EnableModulator"){
          int enable = value.toInt();
          if(enable == 0){
            timbreData[i].enable_modulator = false;
          }else{
            timbreData[i].enable_modulator = true;
          }
        }else if(key == "TypeModulator"){
          timbreData[i].type_modulator = value.toInt();
        }else if(key == "VolumeModulator"){
          timbreData[i].base_volume_modulator = value.toInt();
        }else if(key == "WaveformTypeModulator"){
          int type = value.toInt();
          if(0 <= type && type < WAVEFORMS){
            timbreData[i].waveform_type_modulator = value.toInt();
          }
        }else if(key == "FrequencyRatio"){
          timbreData[i].frequency_ratio = value.toFloat();
        }else if(key == "EnableEnvelopeModulator"){
          int envelope = value.toInt();
          if(envelope == 0){
            timbreData[i].enable_envelope_modulator = false;
          }else{
            timbreData[i].enable_envelope_modulator = true;
          }
        }else if(key == "TicksAttackModulator"){
          int attack = value.toInt();
          if(attack != 0){
            timbreData[i].ticks_attack_modulator = attack;
          }
        }else if(key == "TicksDecayModulator"){
          int decay = value.toInt();
          if(decay != 0){
            timbreData[i].ticks_decay_modulator = decay;
          }
        }else if(key == "VolumeSustainModulator"){
          timbreData[i].volume_sustain_modulator = value.toInt();
        }else if(key == "TicksReleaseModulator"){
          int release_ = value.toInt();
          if(release_ != 0){
            timbreData[i].ticks_release_modulator = release_;
          }
        }
      }
  
      file.close();
      /*
      Serial.println("TimbreDataIndex:" + String(i));
      Serial.println("WaveformType            :" + String(timbreData[i].waveform_type));
      Serial.println("EnableEnvelope          :" + String(timbreData[i].enable_envelope));
      Serial.println("TicksAttack             :" + String(timbreData[i].ticks_attack));
      Serial.println("TicksDecay              :" + String(timbreData[i].ticks_decay));
      Serial.println("VolumeSustain           :" + String(timbreData[i].volume_sustain));
      Serial.println("TicksRelease            :" + String(timbreData[i].ticks_release));
      Serial.println("EnableModulator         :" + String(timbreData[i].enable_modulator));
      Serial.println("TypeModulator           :" + String(timbreData[i].type_modulator));
      Serial.println("VolumeModulator         :" + String(timbreData[i].base_volume_modulator));
      Serial.println("WaveformTypeModulator   :" + String(timbreData[i].waveform_type_modulator));
      Serial.println("FrequencyRatio          :" + String(timbreData[i].frequency_ratio,3));
      Serial.println("EnableEnvelopeModulator :" + String(timbreData[i].enable_envelope_modulator));
      Serial.println("TicksAttackModulator    :" + String(timbreData[i].ticks_attack_modulator));
      Serial.println("TicksDecayModulator     :" + String(timbreData[i].ticks_decay_modulator));
      Serial.println("VolumeSustainModulator  :" + String(timbreData[i].volume_sustain_modulator));
      Serial.println("TicksReleaseModulator   :" + String(timbreData[i].ticks_release_modulator));
      */
    }else{
      //Serial.println(filename + ":Failed to open file");
    }
  }
}

void setTimbreData(uint16_t channel_index ,uint16_t tone_index){
  
  trackData[channel_index].waveform_type = timbreData[tone_index].waveform_type;
  trackData[channel_index].enable_envelope = timbreData[tone_index].enable_envelope;
  trackData[channel_index].ticks_attack = timbreData[tone_index].ticks_attack;
  trackData[channel_index].ticks_decay = timbreData[tone_index].ticks_decay;
  trackData[channel_index].volume_sustain = timbreData[tone_index].volume_sustain;
  trackData[channel_index].ticks_release = timbreData[tone_index].ticks_release;
  trackData[channel_index].enable_modulator = timbreData[tone_index].enable_modulator;
  trackData[channel_index].type_modulator = timbreData[tone_index].type_modulator;
  trackData[channel_index].base_volume_modulator = timbreData[tone_index].base_volume_modulator;
  trackData[channel_index].waveform_type_modulator = timbreData[tone_index].waveform_type_modulator;
  trackData[channel_index].frequency_ratio = timbreData[tone_index].frequency_ratio;
  trackData[channel_index].enable_envelope_modulator = timbreData[tone_index].enable_envelope_modulator;
  trackData[channel_index].ticks_attack_modulator = timbreData[tone_index].ticks_attack_modulator;
  trackData[channel_index].ticks_decay_modulator = timbreData[tone_index].ticks_decay_modulator;
  trackData[channel_index].volume_sustain_modulator = timbreData[tone_index].volume_sustain_modulator;
  trackData[channel_index].ticks_release_modulator = timbreData[tone_index].ticks_release_modulator;
  
}

inline char getChar(uint16_t channel_index){
  
  if(trackData[channel_index].mml_index == MML_BUFFER_SIZE){
    Serial.println("Out of buffer.");
    while(true){
      delay(100);
    }
  }
  
  char c = trackData[channel_index].mml_buffer[trackData[channel_index].mml_index];
  
  if(c != '\0'){
    trackData[channel_index].mml_index++;
  }
  /*
  if(channel_index == 0){
    Serial.print(c);
  }
  */
  return toupper(c);
}

#define BEAT_TICKS      480
#define DEFAULT_TEMPO     60
#define MAX_TEMPO         255
#define MIN_TEMPO         1
#define SECONDS_IN_MINUTE 60.0
uint16_t tickTimeMilli;
uint16_t tickTimeMicro;
void setTickTime(uint16_t tempo){
  double beat_duration;
  double tick_duration;
  
  //Serial.println(tempo);
  
  if(MIN_TEMPO <= tempo && tempo <= MAX_TEMPO){
    beat_duration = SECONDS_IN_MINUTE / tempo;
  }else{
    beat_duration = SECONDS_IN_MINUTE / DEFAULT_TEMPO;
  }
  
  tick_duration = beat_duration / BEAT_TICKS;
  
  /*
  Serial.println(beat_duration);
  Serial.println(tick_duration,6);
  Serial.println(tick_duration,12);
  */
  tickTimeMilli = tick_duration * 1000;
  tickTimeMicro = (tick_duration * 1000000) - (tickTimeMilli * 1000);
  /*
  Serial.println(tickTimeMilli);
  Serial.println(tickTimeMicro);
  */
}

bool isNumber(char c){
  
  if(c >= 48 && c <= 57){
    return true;
  }
  
  return false;
}

#define TEMPO_NUM_LENGTH_MAX  3
void setTempo(uint16_t channel_index){
  char      c;
  char      temp[TEMPO_NUM_LENGTH_MAX + 1] = {};
  uint16_t tempo;
  
  for(uint16_t i=0;i < TEMPO_NUM_LENGTH_MAX;i++){
    c = getChar(channel_index);
    if(isNumber(c)){
      temp[i] = c;
    }else{
      trackData[channel_index].mml_index--;
      break;
    }
  }
  
  tempo = atoi(temp);
  
  if(tempo != 0){
    setTickTime(tempo);
  }
}

void setOctave(uint16_t channel_index){
  char c;
  
  c = getChar(channel_index);
  if(isNumber(c)){
    trackData[channel_index].octave = c - '0';
  }else{
    trackData[channel_index].mml_index--;
  }
  //Serial.println("octave:" + String(trackData[channel_index].octave));
}

#define MAX_OCTAVE  8
void incrementOctave(uint16_t channel_index){
  
  if(trackData[channel_index].octave < MAX_OCTAVE){
    trackData[channel_index].octave++;
  }
  //Serial.println("octave:" + String(trackData[channel_index].octave));
}

#define MIN_OCTAVE  0
void decrementOctave(uint16_t channel_index){
  
  if(trackData[channel_index].octave > MIN_OCTAVE){
    trackData[channel_index].octave--; 
  }
  //Serial.println("octave:" + String(trackData[channel_index].octave));
}

uint16_t getTicks(uint16_t duration){
  uint16_t value = BEAT_TICKS * 4;  // Whole Note
  return value / duration;
}


#define DEFAULT_TICKS_NUM_LENGTH_MAX  2
uint16_t defaultTicks;
void setDefaultTicks(uint16_t channel_index){
  char      c;
  char      temp[DEFAULT_TICKS_NUM_LENGTH_MAX + 1] = {};
  uint16_t  duration;
  
  for(uint16_t i=0;i < DEFAULT_TICKS_NUM_LENGTH_MAX;i++){
    c = getChar(channel_index);
    if(isNumber(c)){
      temp[i] = c;
    }else{
      trackData[channel_index].mml_index--;
      break;
    }
  }
  
  duration = atoi(temp);
  
  switch(duration){
    case 1:
    case 2:
    case 4:
    case 8:
    case 16:
    case 32:
      defaultTicks = getTicks(duration);
      break;
    default:
      break;
  }
}


bool isNoteNum(uint16_t duration){
    switch(duration){
      case 1:
      case 2:
      case 4:
      case 6:
      case 8:
      case 12:
      case 16:
      case 24:
      case 32:
      case 48:
        return true;
      default:
        return false;
    }
}

#define DOTTED_TICKS_MIN  30
uint16_t getDottedTicks(uint16_t channel_index ,uint16_t base_ticks){
  char c;
  uint16_t pre_ticks = base_ticks;
  uint16_t add_ticks;
  uint16_t ticks = base_ticks;
  
  if(base_ticks > DOTTED_TICKS_MIN){
    
    trackData[channel_index].mml_index--;
    while(pre_ticks > DOTTED_TICKS_MIN){
      c = getChar(channel_index);
      if(c == '.'){
        add_ticks = pre_ticks / 2;
        ticks = ticks + add_ticks;
        pre_ticks = add_ticks;
      }else{
        trackData[channel_index].mml_index--;
        break;
      }
    }
  }
  
  return ticks;
}

#define NOTE_DURATION_LENGTH_MAX  2
void addTieTicks(uint16_t channel_index,char note,char half_step){
  char c;
  uint16_t duration;
  uint16_t ticks;
  
  trackData[channel_index].mml_index--;
  while(trackData[channel_index].ticks < UINT32_MAX - (BEAT_TICKS * 4)){
    c = getChar(channel_index);
    
    if(c == '&'){
      
      c = getChar(channel_index);
      if(c == note){
        
        c = getChar(channel_index);
        
        if(c == half_step){
          c = getChar(channel_index);
        }
        
        if(isNumber(c)){
          trackData[channel_index].mml_index--;
          
          char temp[NOTE_DURATION_LENGTH_MAX + 1] = {};
          for(uint16_t i=0;i < NOTE_DURATION_LENGTH_MAX;i++){
            c = getChar(channel_index);
            if(isNumber(c)){
              temp[i] = c;
            }else{
              trackData[channel_index].mml_index--;
            }
          }
          
          duration = atoi(temp);
          if(isNoteNum(duration)){
            ticks = getTicks(duration);
          }else{
            return;
          }
          
          c = getChar(channel_index);
          if(c == '.'){
            ticks = getDottedTicks(channel_index,ticks);
          }else{
            trackData[channel_index].mml_index--;
          }
        }else if(c == '.'){
          ticks = getDottedTicks(channel_index,defaultTicks);
        }else{
          ticks = defaultTicks;
          trackData[channel_index].mml_index--;
        }
        
        trackData[channel_index].ticks = trackData[channel_index].ticks + ticks;
        
      }else{
        trackData[channel_index].mml_index--;
        break;
      }
      
    }else{
      trackData[channel_index].mml_index--;
      break;
    }
  }
  
  //Serial.println(String(note) + ":" + String(trackData[channel_index].ticks));
}

#define ATTACK                0
#define NOTE_INDEX_LENGTH_MAX 2
uint16_t phaseCarrier[CHANNELS];
uint16_t phaseModulator[CHANNELS];
bool setNote(uint16_t channel_index,char note){
  char c;
  char half_step = '\0';
  uint16_t index;
  uint16_t octave = trackData[channel_index].octave;
  
  if(note == 'R'){
    
    trackData[channel_index].frequency = 0;
    
  }else{
    
    if(octave == MIN_OCTAVE){
      switch(note){
      case 'A':
        index = 0;
        break;
      case 'B':
        index = 2;
        break;
      default:
        return false;
      }
    }else if(octave == MAX_OCTAVE){
      if(note == 'C'){
        index = NOTES - 1;
      }else{
        return false;
      }
    }else{
      
      switch(note){
        case 'C':
          index = 3;
          break;
        case 'D':
          index = 5;
          break;
        case 'E':
          index = 7;
          break;
        case 'F':
          index = 8;
          break;
        case 'G':
          index = 10;
          break;
        case 'A':
          index = 12;
          break;
        case 'B':
          index = 14;
          break;
      }
      
      index = ((octave - 1) * 12) + index;
    }
    
    c = getChar(channel_index);
    if(c == '+' || c == '#'){
      half_step = c;
      if(index < NOTES){
        index++;
      }else{
        return false;
      }
    }else if(c == '-'){
      half_step = c;
      if(index > 0){
        index--;
      }else{
        return false;
      }
    }else{
      if(c != '\0'){
        trackData[channel_index].mml_index--;
      }
    }
    
    trackData[channel_index].frequency = frequencyData[index];
    trackData[channel_index].frequency_modulator = trackData[channel_index].frequency * trackData[channel_index].frequency_ratio;
    phaseCarrier[channel_index] = 0;
    phaseModulator[channel_index] = 0;
  }
  //Serial.println("frequency:" + String(trackData[channel_index].frequency));
  //Serial.println("frequency_modulator:" + String(trackData[channel_index].frequency_modulator));
  
  c = getChar(channel_index);
  if(isNumber(c)){
    trackData[channel_index].mml_index--;
    
    char temp[NOTE_DURATION_LENGTH_MAX + 1] = {};
    for(uint16_t i=0;i < NOTE_DURATION_LENGTH_MAX;i++){
      c = getChar(channel_index);
      if(isNumber(c)){
        temp[i] = c;
      }else{
        trackData[channel_index].mml_index--;
      }
    }
    
    uint16_t duration = atoi(temp);
    /*
    if(channel_index == 4){
      Serial.println(String(note) + String(duration));
    }
    */
    if(isNoteNum(duration)){
      trackData[channel_index].ticks = getTicks(duration);
    }else{
      return false;
    }
    
    c = getChar(channel_index);
    if(c == '.'){
      trackData[channel_index].ticks = getDottedTicks(channel_index,trackData[channel_index].ticks);
      
      c = getChar(channel_index);
      if(c == '&'){
        addTieTicks(channel_index,note,half_step);
      }else{
        trackData[channel_index].mml_index--;
      }
      
    }else if(c == '&'){
      addTieTicks(channel_index,note,half_step);
    }else{
      trackData[channel_index].mml_index--;
    }
    
  }else if(c == '.'){
    trackData[channel_index].ticks = getDottedTicks(channel_index,defaultTicks);
    
    c = getChar(channel_index);
    if(c == '&'){
      addTieTicks(channel_index,note,half_step);
    }else{
      trackData[channel_index].mml_index--;
    }
    
  }else if(c == '&'){
    trackData[channel_index].ticks = defaultTicks;
    addTieTicks(channel_index,note,half_step);
  }else{
    trackData[channel_index].ticks = defaultTicks;
    if(c != '\0'){
      trackData[channel_index].mml_index--;
    }
  }
  
  /*
  if(channel_index == 0){
    Serial.println("ticks:" + String(trackData[channel_index].ticks));
  }
  */
  
  trackData[channel_index].tick = 1;
  trackData[channel_index].status_adsr = ATTACK;
  trackData[channel_index].status_adsr_modulator = ATTACK;
  trackData[channel_index].enable = true;
  
  return true;
}

#define DEFAULT_TIMBRE_NUM_LENGTH_MAX 2
void setTimbre(uint16_t channel_index){
  char c;
  char temp[DEFAULT_TIMBRE_NUM_LENGTH_MAX + 1] = {};
  uint16_t index;
  
  for(uint16_t i=0;i < DEFAULT_TIMBRE_NUM_LENGTH_MAX;i++){
    c = getChar(channel_index);
    if(isNumber(c)){
      temp[i] = c;
    }else{
      trackData[channel_index].mml_index--;
      break;
    }
  }
  
  index = atoi(temp);
  
  setTimbreData(channel_index,index);
  //Serial.println("channel:" + String(channel_index) + " Timbre:" + String(index));
}


#define DEFAULT_VOLUME_NUM_LENGTH_MAX  3
void setBaseVolume(uint16_t channel_index){
  char      c;
  char      temp[DEFAULT_VOLUME_NUM_LENGTH_MAX + 1] = {};
  uint16_t  volume;
  
  for(uint16_t i=0;i < DEFAULT_VOLUME_NUM_LENGTH_MAX;i++){
    c = getChar(channel_index);
    if(isNumber(c)){
      temp[i] = c;
    }else{
      trackData[channel_index].mml_index--;
      break;
    }
  }
  
  volume = atoi(temp);
  
  if(volume >= 0 && volume <= 100){
    trackData[channel_index].base_volume = volume;
  }
  
}

bool setNoteOn;
uint32_t trackPlayIdx = CHANNELS;
void setTrackData(uint16_t channel_index){
  char c;
  bool finish;
  bool comment;
  
  while(finish == false){
    c = getChar(channel_index);
    if(comment){
      if(c == '\n' || c == '\0'){
        comment = false;
      }
    }else{
      switch(c){
        case ';':
          comment = true;
          break;
        case 'T':
          setTempo(channel_index);
          break;
        case '>':
          incrementOctave(channel_index);
          break;
        case '<':
          decrementOctave(channel_index);
          break;
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'A':
        case 'B':
        case 'R':
          finish = setNote(channel_index,c);
          setNoteOn = true;
          break;
        case '@':
          if(trackPlayIdx == CHANNELS){
            setTimbre(channel_index);
          }
          break;
        case 'O':
          setOctave(channel_index);
          break;
        case 'L':
          setDefaultTicks(channel_index);
          break;
        case 'V':
          setBaseVolume(channel_index);
          break;
        case '\0':
          finish = true;
          break;
      }
    }
    /*
    if(channel_index == 1){
      if(finish){
        Serial.println("finish");
      }else{
        Serial.println("next");
      }
    }
    */
  }
}

TimbreData tmpTimbreData;
bool adsrBusy = false;
bool updateTimbre = false;
void updateTrackTimbre(uint16_t index){
  while(adsrBusy){delayMicroseconds(1);}
  updateTimbre = true;
  
  trackData[index].waveform_type = tmpTimbreData.waveform_type;
  trackData[index].enable_envelope = tmpTimbreData.enable_envelope;
  trackData[index].ticks_attack = tmpTimbreData.ticks_attack;
  trackData[index].ticks_decay = tmpTimbreData.ticks_decay;
  trackData[index].volume_sustain = tmpTimbreData.volume_sustain;
  trackData[index].ticks_release = tmpTimbreData.ticks_release;
  trackData[index].enable_modulator = tmpTimbreData.enable_modulator;
  trackData[index].type_modulator = tmpTimbreData.type_modulator;
  trackData[index].base_volume_modulator = tmpTimbreData.base_volume_modulator;
  trackData[index].waveform_type_modulator = tmpTimbreData.waveform_type_modulator;
  trackData[index].frequency_ratio = tmpTimbreData.frequency_ratio;
  trackData[index].enable_envelope_modulator = tmpTimbreData.enable_envelope_modulator;
  trackData[index].ticks_attack_modulator = tmpTimbreData.ticks_attack_modulator;
  trackData[index].ticks_decay_modulator = tmpTimbreData.ticks_decay_modulator;
  trackData[index].volume_sustain_modulator = tmpTimbreData.volume_sustain_modulator;
  trackData[index].ticks_release_modulator = tmpTimbreData.ticks_release_modulator;
  
}

#define DEFAULT_TIMBRE      0
#define DEFAULT_OCTAVE      4
#define DEFAULT_BASE_VOLUME 75
void initializeTrackData(){

  setTickTime(0);
  defaultTicks = BEAT_TICKS;
  
  for(int i=0;i < CHANNELS;i++){
    trackData[i].mml_index = 0;
    trackData[i].mml_count = mmlFile[i].available();
    trackData[i].octave = DEFAULT_OCTAVE;
    trackData[i].base_volume = DEFAULT_BASE_VOLUME;
    //trackData[i].base_volume_modulator = DEFAULT_BASE_VOLUME;
    setTimbreData(i,DEFAULT_TIMBRE);
  }
  
  initializeMmlBuffer();

  if(trackPlayIdx == CHANNELS){
    for(int i=0;i < CHANNELS;i++){
      setTrackData(i);
    }
  }else{
    updateTrackTimbre(trackPlayIdx);
    setTrackData(trackPlayIdx);
  }
}

bool finishSetup = false;
WebServer server(80);
#define STAT_STOP     0
#define STAT_STOPPING 1
#define STAT_PLAY     2
#define STAT_PAUSE    3
uint32_t playStatus = STAT_STOP;
void setup1(){
  while(finishSetup == false){
    delay(100);
  }
  initializeFrequency();
  initializeWaveform();
  initializeCalculateAmplitudeTable();
  
  if (!LittleFS.begin()) {
    Serial.println("LittleFS initialization failed.");
    while(true){
      delay(100);
    }
  }
  
  pinMode(LED_BUILTIN, OUTPUT);
  
  // Serve HTML with image
  server.on("/", HTTP_GET, handleRoot);
  server.on("/script.js", HTTP_GET, handleScript);
  server.on("/style.css", HTTP_GET, handleStyle);
  server.on("/player.html", HTTP_GET, handlePlayer);
  server.on("/mml.html", HTTP_GET, handleMml);
  server.on("/timbre.html", HTTP_GET, handleTimbre);
  server.on("/api", HTTP_POST, handleAPI);
  
  // Start server
  server.begin();
  
  digitalWrite(LED_BUILTIN, HIGH);

  
  while(playStatus == STAT_STOP){
    delay(100);
  }
  Serial.println("Start Play");
  initializeTimbreData();
  initializeMmlFile();
  initializeTrackData();
  
}

void setMmlBuffer(uint16_t channel_index){
  
  if(trackData[channel_index].mml_index == 0){
    return;
  }
  uint16_t  readbyte;
  if(trackData[channel_index].mml_count <= trackData[channel_index].mml_index){
    readbyte = trackData[channel_index].mml_count;
  }else{
    readbyte = trackData[channel_index].mml_index;
  }
  
  uint16_t start_index;
  for(uint16_t i=0;i < MML_BUFFER_SIZE;i++){
    if(trackData[channel_index].mml_index + i >= MML_BUFFER_SIZE){
      break;
    }
    trackData[channel_index].mml_buffer[i] = trackData[channel_index].mml_buffer[trackData[channel_index].mml_index + i];
    start_index = i + 1;
  }
  
  uint8_t   buffer[MML_BUFFER_SIZE];
  mmlFile[channel_index].read(buffer,readbyte);
  
  uint16_t j = 0;
  for(uint16_t i=start_index;i < MML_BUFFER_SIZE;i++){
    if(j > readbyte - 1){
      trackData[channel_index].mml_buffer[i] = '\0';
      break;
    }else{
      trackData[channel_index].mml_buffer[i] = buffer[j];
      j++;
    }
  }
  
  /*
  for(int i=0;i < CHANNELS;i++){
    Serial.println("track_" + String(i));
    for(int j=0;j < MML_BUFFER_SIZE;j++){
      //int c = trackData[i].mml_buffer[j];
      //Serial.print("(");
      //Serial.print(c);
      //Serial.print(")");
      Serial.print(trackData[i].mml_buffer[j]);
    }
    Serial.println(":END");
  }
  while(true){delay(100);}
  */
  
  
  trackData[channel_index].mml_count = trackData[channel_index].mml_count - readbyte;
  trackData[channel_index].mml_index = 0;
}

void tickDelay(unsigned long elapsed_time_micro){
  unsigned long correct_time_mill;
  unsigned long correct_time_micro;
  bool borrow = false;
  /*
  Serial.println("tickTimeMilli:" + String(tickTimeMilli));
  Serial.println("tickTimeMicro:" + String(tickTimeMicro));
  */
  correct_time_mill = elapsed_time_micro / 1000;
  correct_time_micro = elapsed_time_micro % 1000;
  /*
  Serial.println("correct_time_mill:" + String(correct_time_mill));
  Serial.println("correct_time_micro:" + String(correct_time_micro));
  */
  
  if(correct_time_mill > tickTimeMilli){
    Serial.println("Processing time exceeded.");
    correct_time_mill = tickTimeMilli;
    correct_time_micro = tickTimeMicro;
    
  }else if(correct_time_mill == tickTimeMilli){
    if(correct_time_micro > tickTimeMicro){
      Serial.println("Processing time exceeded.");
      correct_time_mill = tickTimeMilli;
      correct_time_micro = tickTimeMicro;
    }
  }else if(correct_time_mill < tickTimeMilli){
    if(correct_time_micro > tickTimeMicro){
      borrow = true;
    }
  }
  
  uint16_t wait_mill;
  uint16_t wait_micro;
  if(borrow){
    wait_mill = tickTimeMilli - correct_time_mill - 1;
    wait_micro = 1000 + tickTimeMicro - correct_time_micro;
  }else{
    wait_mill = tickTimeMilli - correct_time_mill;
    wait_micro = tickTimeMicro - correct_time_micro;
  }
  
  /*
  Serial.println("wait_mill:" + String(wait_mill));
  Serial.println("wait_micro:" + String(wait_micro));
  //while(true){delay(100);}
  */
  
  delay(wait_mill);
  delayMicroseconds(wait_micro);
}

#define DECAY   1
#define SUSTAIN 2
#define RELEASE 3
uint32_t channelIndex;
unsigned long startTime;
unsigned long completionTime;
unsigned long elapsedTime;
uint32_t skipTicks;
void loop1() {
  
  //timeChecker.start();
  startTime = micros();
  
  setNoteOn = false;
  if(trackPlayIdx == CHANNELS){
    for(uint16_t i=0;i < CHANNELS;i++){
      if(trackData[i].enable == false){
        setTrackData(i);
      }
    }
  }else{
    if(trackData[trackPlayIdx].enable == false){
      setTrackData(trackPlayIdx);
      updateTimbre = false;
    }
  }
  
  bool flagPlaying = false;
  for(uint32_t i=0;i<CHANNELS;i++){
    if(trackData[i].enable == true){
      flagPlaying = true;
      break;
    }
  }
  if(flagPlaying == false){
    playStatus = STAT_STOPPING;
  }
  
  if(setNoteOn == false){
    setMmlBuffer(channelIndex);
  }
  
  adsrBusy = true;
  for(uint16_t i=0;i < CHANNELS;i++){
    
    if(trackData[i].frequency == 0){
      trackData[i].volume = 0;
    }else if(trackData[i].enable_envelope && updateTimbre == false){
      
      if(trackData[i].status_adsr == ATTACK){
        trackData[i].volume = trackData[i].base_volume * (trackData[i].tick * 100 / trackData[i].ticks_attack) / 100;
        if(trackData[i].tick == trackData[i].ticks_attack){
          trackData[i].status_adsr = DECAY;
        }
      }else if(trackData[i].status_adsr == DECAY){
        uint16_t volume_sustain = trackData[i].base_volume * trackData[i].volume_sustain / 100;
        if(trackData[i].base_volume > volume_sustain){
          trackData[i].volume = trackData[i].base_volume - ((trackData[i].base_volume - volume_sustain) * ((trackData[i].tick - trackData[i].ticks_attack) * 100 / trackData[i].ticks_decay) / 100);
        }
        if(trackData[i].tick == trackData[i].ticks_attack + trackData[i].ticks_decay){
          trackData[i].status_adsr = SUSTAIN;
          if(trackData[i].ticks <= trackData[i].ticks_attack + trackData[i].ticks_decay + trackData[i].ticks_release){
            trackData[i].status_adsr = RELEASE;
          }
        }
      }else if(trackData[i].status_adsr == SUSTAIN){
        if(trackData[i].tick == trackData[i].ticks - trackData[i].ticks_release - 1){
            trackData[i].status_adsr = RELEASE;
        }
      }else if(trackData[i].status_adsr == RELEASE){
        uint16_t volume_sustain = trackData[i].base_volume * trackData[i].volume_sustain / 100;
        uint16_t ticks_attack_decay = trackData[i].ticks_attack + trackData[i].ticks_decay;
        uint16_t tick;
        if(trackData[i].ticks <= (ticks_attack_decay + trackData[i].ticks_release)){
          tick = trackData[i].tick - ticks_attack_decay;
        }else{
          tick = trackData[i].tick - (trackData[i].ticks - trackData[i].ticks_release);
        }
        trackData[i].volume = volume_sustain - (volume_sustain * (tick * 100 / trackData[i].ticks_release) / 100);
        
      }
      
    }else if(trackData[i].enable_envelope == false){
      trackData[i].volume = trackData[i].base_volume;
    }
    
    
    if(trackData[i].frequency_modulator == 0){
      trackData[i].volume_modulator = 0;
    }else if(trackData[i].enable_envelope_modulator && updateTimbre == false){
      
      if(trackData[i].status_adsr_modulator == ATTACK){
        trackData[i].volume_modulator = trackData[i].base_volume_modulator * (trackData[i].tick * 100 / trackData[i].ticks_attack_modulator) / 100;
        if(trackData[i].tick == trackData[i].ticks_attack_modulator){
          trackData[i].status_adsr_modulator = DECAY;
        }
      }else if(trackData[i].status_adsr_modulator == DECAY){
        uint16_t volume_sustain = trackData[i].base_volume_modulator * trackData[i].volume_sustain_modulator / 100;
        if(trackData[i].base_volume_modulator > volume_sustain){
          trackData[i].volume_modulator = trackData[i].base_volume_modulator - ((trackData[i].base_volume_modulator - volume_sustain) * ((trackData[i].tick - trackData[i].ticks_attack_modulator) * 100 / trackData[i].ticks_decay_modulator) / 100);
        }
        if(trackData[i].tick == trackData[i].ticks_attack_modulator + trackData[i].ticks_decay_modulator){
          trackData[i].status_adsr_modulator = SUSTAIN;
          if(trackData[i].ticks <= trackData[i].ticks_attack_modulator + trackData[i].ticks_decay_modulator + trackData[i].ticks_release_modulator){
            trackData[i].status_adsr_modulator = RELEASE;
          }
        }
      }else if(trackData[i].status_adsr_modulator == SUSTAIN){
        if(trackData[i].tick == trackData[i].ticks - trackData[i].ticks_release_modulator - 1){
            trackData[i].status_adsr_modulator = RELEASE;
        }
      }else if(trackData[i].status_adsr_modulator == RELEASE){
        uint16_t volume_sustain = trackData[i].base_volume_modulator * trackData[i].volume_sustain_modulator / 100;
        uint16_t ticks_attack_decay = trackData[i].ticks_attack_modulator + trackData[i].ticks_decay_modulator;
        uint16_t tick;
        if(trackData[i].ticks <= (ticks_attack_decay + trackData[i].ticks_release_modulator)){
          tick = trackData[i].tick - ticks_attack_decay;
        }else{
          tick = trackData[i].tick - (trackData[i].ticks - trackData[i].ticks_release_modulator);
        }
        trackData[i].volume_modulator = volume_sustain - (volume_sustain * (tick * 100 / trackData[i].ticks_release_modulator) / 100);
        
      }
      
    }else if(trackData[i].enable_envelope_modulator == false){
      trackData[i].volume_modulator = trackData[i].base_volume_modulator;
    }
  }
  adsrBusy = false;
  
  
  for(uint32_t i=0;i<CHANNELS;i++){
    if(trackData[i].tick >= trackData[i].ticks){
      trackData[i].enable = false;
    }
  }
  
  
  for(uint32_t i=0;i<CHANNELS;i++){
    if(trackData[i].enable){
      trackData[i].tick++;
    }
  }
  
  channelIndex++;
  if(channelIndex >= CHANNELS){
    channelIndex = 0;
  }
  
  completionTime = micros();
  if(completionTime >= startTime){
    elapsedTime = completionTime - startTime;
  }else{
    elapsedTime = (ULONG_MAX - startTime) + completionTime + 1;
  }
  
  /*
  if(elapsedTime > 300){
    Serial.println("elapsed:" + String(elapsedTime));
    Serial.println("setNoteOn:" + String(setNoteOn));
  }
  */
  if(skipTicks == 0){
    tickDelay(elapsedTime);
  }else{
    skipTicks--;
  }
  
  if(playStatus != STAT_PLAY){
    if(playStatus == STAT_STOPPING){
      Serial.println("Performance completed.");
      for(uint32_t i=0;i<CHANNELS;i++){
        mmlFile[i].close();
        trackData[i].enable = false;
      }
      playStatus = STAT_STOP;
      skipTicks = 0;
      updateTimbre = false;
      while(playStatus == STAT_STOP){
        delay(100);
      }
      Serial.println("Start Play");
      initializeTimbreData();
      initializeMmlFile();
      initializeTrackData();
    }else if(playStatus == STAT_PAUSE){
      while(playStatus == STAT_PAUSE){
        delay(100);
      }
    }
  }
  
  //Serial.println(timeChecker.stop());
}

String rootHTML = ""
  "<!DOCTYPE html>"
  "<html lang='en'>"
  "       <head>"
  "               <meta charset='UTF-8'>"
  "               <meta name='viewport' content='width=device-width, initial-scale=1.0'>"
  "               <title>MML Player Web UI</title>"
  "               <link rel='stylesheet' href='style.css'>"
  "       </head>"
  "       <body>"
  "               <button id='btn_menu'>Menu</button>"
  "               <div id='container'>"
  "                       <div id='menu'>"
  "                               <div id='menu_player' class='menu_item'>Player</div>"
  "                               <div id='menu_mml' class='menu_item'>Edit MML</div>"
  "                               <div id='menu_timbre' class='menu_item'>Edit Timbre</div>"
  "                       </div>"
  "                       <div id='content'>"
  "                               <h1>MML Player</h1>"
  "                               <p>Please select a menu.</p>"
  "                       </div>"
  "               </div>"
  "               <script src='script.js'></script>"
  "       </body>"
  "</html>"
  ;

void handleRoot(){
  server.send(200,"text/html",rootHTML);
}

String rootScript = ""
  "const channels = " + String(CHANNELS) + ";"
  "const min_width = 1079;"
  "var btn_menu = document.getElementById('btn_menu');"
  "var content = document.getElementById('content');"
  "var menu = document.getElementById('menu');"
  "var menu_player = document.getElementById('menu_player');"
  "var menu_mml = document.getElementById('menu_mml');"
  "var menu_timbre = document.getElementById('menu_timbre');"
  ""
  "function onSelectTimbre() {"
  "       var timbre = document.getElementById('timbre');"
  "       var html = 'timbre.html?timbre=' + timbre.value;"
  ""
  "       getForm(html);"
  "}"
  ""
  "function onSliderChangeFRQ() {"
  "       var sliderHigh = document.getElementById('frequency_ratio_high_slider');"
  "       var sliderLow = document.getElementById('frequency_ratio_low_slider');"
  "       var input = document.getElementById('frequency_ratio');"
  "       var value_low = parseInt(sliderLow.value) / 1000.0;"
  "       "
  "       if(sliderHigh.value == 15){"
  "               sliderLow.value = 0;"
  "               input.value = 15;"
  "       }else{"
  "               input.value = (parseFloat(frequency_ratio_high_slider.value) + value_low).toFixed(3);"
  "       }"
  "       "
  "       postTimbre(0);"
  "}"
  ""
  "function onSliderChange(event) {"
  "       var elementId = event.target.id;"
  "       var slider = document.getElementById(elementId);"
  "       elementId = elementId.substring(0, elementId.length - 7);"
  "       var input = document.getElementById(elementId);"
  "       "
  "       input.value = slider.value;"
  "       postTimbre(0);"
  "}"
  ""
  "function onInputChangeFRQ(event) {"
  "       var elementId = event.target.id;"
  "       var input = document.getElementById(elementId);"
  "       var h_slider = document.getElementById(elementId + '_high_slider');"
  "       var l_slider = document.getElementById(elementId + '_low_slider');"
  "       var value = input.value;"
  "       "
  "       var decimal = Math.floor(value);"
  "       h_slider.value = decimal;"
  "       l_slider.value = (value - decimal) * 1000;"
  "       "
  "       postTimbre(0);"
  "}"
  ""
  "function onInputChange(event) {"
  "       var elementId = event.target.id;"
  "       var input = document.getElementById(elementId);"
  "       elementId = elementId + '_slider';"
  "       var slider = document.getElementById(elementId);"
  "       "
  "       slider.value = input.value;"
  "       postTimbre(0);"
  "}"
  ""
  "function getForm(html) {"
  "       fetch(html)"
  "       .then(response => response.text())"
  "       .then(data => {"
  "               content.innerHTML = data;"
  "       })"
  "       .catch(error => {"
  "               console.error('error:', error);"
  "       });"
  "}"
  ""
  "function updateDisplay() {"
  "       if (window.innerWidth <= min_width) {"
  "               btn_menu.style.display = 'block';"
  "               menu.style.display = 'none';"
  "               content.style.display = 'block';"
  "       } else {"
  "               btn_menu.style.display = 'none';"
  "               menu.style.display = 'block';"
  "               content.style.display = 'block';"
  "       }"
  "}"
  ""
  "function appendTimbre(formData) {"
  "       "
  "       formData.append('description', document.getElementById('description').value);"
  "       formData.append('waveform_type', document.getElementById('waveform_type').value);"
  "       formData.append('enable_envelope', document.getElementById('enable_envelope').value);"
  "       formData.append('ticks_attack', document.getElementById('ticks_attack').value);"
  "       formData.append('ticks_decay', document.getElementById('ticks_decay').value);"
  "       formData.append('volume_sustain', document.getElementById('volume_sustain').value);"
  "       formData.append('ticks_release', document.getElementById('ticks_release').value);"
  "       formData.append('enable_modulator', document.getElementById('enable_modulator').value);"
  "       formData.append('type_modulator', document.getElementById('type_modulator').value);"
  "       formData.append('waveform_type_modulator', document.getElementById('waveform_type_modulator').value);"
  "       formData.append('base_volume_modulator', document.getElementById('base_volume_modulator').value);"
  "       formData.append('frequency_ratio', document.getElementById('frequency_ratio').value);"
  "       formData.append('enable_envelope_modulator', document.getElementById('enable_envelope_modulator').value);"
  "       formData.append('ticks_attack_modulator', document.getElementById('ticks_attack_modulator').value);"
  "       formData.append('ticks_decay_modulator', document.getElementById('ticks_decay_modulator').value);"
  "       formData.append('volume_sustain_modulator', document.getElementById('volume_sustain_modulator').value);"
  "       formData.append('ticks_release_modulator', document.getElementById('ticks_release_modulator').value);"
  "       "
  "       return formData;"
  "}"
  ""
  "function postPlayer(state) {"
  "       var xhr = new XMLHttpRequest();"
  "       xhr.open('POST', 'api', true);"
  "       var track = document.getElementById('track');"
  "       var skip = document.getElementById('skip');"
  "       "
  "       var formData = new FormData();"
  "       formData.append('method', 'setPlayer');"
  "       formData.append('status', state);"
  "       if(skip !== null){"
  "               formData.append('skip', skip.value);"
  "       }"
  "       if(track !== null){"
  "               formData.append('track', track.value);"
  "               formData = appendTimbre(formData);"
  "       }"
  "       "
  "       xhr.send(formData);"
  "}"
  ""
  "function postMml() {"
  "       var xhr = new XMLHttpRequest();"
  "       xhr.open('POST', 'api', true);"
  "       "
  "       var formData = new FormData();"
  "       "
  "       formData.append('method', 'setMml');"
  "       for(var i=1;i <= channels;i++){"
  "               var number = '0' + i;"
  "               number = number.substr(-2);"
  "               var id = 'track' + number;"
  "               var content = document.getElementById(id).value;"
  "               formData.append(id, content);"
  "       }"
  "       "
  "       xhr.send(formData);"
  "}"
  ""
  "function postTimbre(state) {"
  "       var elementId = event.target.id;"
  "       var input = document.getElementById(elementId);"
  "       var xhr = new XMLHttpRequest();"
  "       xhr.open('POST', 'api', true);"
  "       "
  "       var formData = new FormData();"
  "       "
  "       formData.append('method', 'setTimbre');"
  "       if(state == 1){"
  "               formData.append('upload', '1');"
  "               formData.append('timbre', document.getElementById('timbre').value);"
  "       }else{"
  "               formData.append('track', document.getElementById('track').value);"
  "       }"
  "       formData = appendTimbre(formData);"
  "       xhr.send(formData);"
  "}"
  ""
  "btn_menu.addEventListener('click', function () {"
  "       if(menu.style.display === 'none' || menu.style.display === ''){"
  "               menu.style.display = 'block';"
  "               content.style.display = 'none';"
  "               btn_menu.style.display = 'none';"
  "       }else{"
  "               menu.style.display = 'none';"
  "               content.style.display = 'block';"
  "               btn_menu.style.display = 'block';"
  "       }"
  "});"
  ""
  "menu.addEventListener('click', function () {"
  "    if (window.innerWidth <= min_width) {"
  "        btn_menu.click();"
  "    }"
  "});"
  ""
  "menu_player.addEventListener('click', () => {"
  "       getForm('player.html');"
  "});"
  ""
  "menu_mml.addEventListener('click', () => {"
  "       getForm('mml.html');"
  "});"
  ""
  "menu_timbre.addEventListener('click', () => {"
  "       getForm('timbre.html');"
  "});"
  ""
  "window.addEventListener('resize', updateDisplay);"
  ""
  "updateDisplay();"
  ;

void handleScript() {
  server.send(200,"application/javascript",rootScript);
}

String rootCSS = ""
  "body {"
  "       margin: 0;"
  "       font-family: Arial, sans-serif;"
  "       background-color: #f4f4f4;"
  "}"
  ""
  "#btn_menu {"
  "       position: fixed;"
  "       top: 20px;"
  "       right: 20px;"
  "}"
  ""
  "#container {"
  "       display: flex;"
  "}"
  ""
  "#menu {"
  "       background-color: #333;"
  "       color: #fff;"
  "       padding: 20px;"
  "       white-space: nowrap;"
  "}"
  ""
  "#content {"
  "    flex-grow: 1;"
  "    padding: 20px;"
  "}"
  ""
  ".menu_item {"
  "       cursor: pointer;"
  "       padding: 10px 0;"
  "}"
  ""
  ".menu_item:hover {"
  "       background-color: #555;"
  "}"
  ""
  ".track {"
  "       float: left;"
  "       margin: 10px;"
  "}"
  ""
  ".track textarea {"
  "       width: 270px;"
  "       height: 520px;"
  "}"
  ""
  "#timbre_player {"
  "       display: inline-block;"
  "       padding: 5px;"
  "       border: double;"
  "}"
  ""
  ".timbre {"
  "       width: 540px;"
  "       float: left;"
  "       margin: 10px;"
  "       display: grid;"
  "       grid-template-columns: repeat(2, 1fr);"
  "       gap: 10px;"
  "}"
  ""
  ".timbre_span {"
  "       grid-column: span 2;"
  "}"
  ""
  "@media only screen and (max-width: 1079px) {"
  ""
  "    #menu {"
  "       width: 100%;"
  "    }"
  ""
  "    #content {"
  "        width: 100%;"
  "    }"
  ""
  "    .track {"
  "        float: none;"
  "    }"
  ""
  "    .track textarea {"
  "        width: 100%;"
  "        margin-right: 0px;"
  "    }"
  ""
  "    .timbre {"
  "       width: auto;"
  "       float: none;"
  "    }"
  ""
  "}"
  ;

void handleStyle() {
  server.send(200,"text/css",rootCSS);
}

String rootPlayerHTML = ""
  "<div>"
  "        <h1>Player Control</h1>"
  "        <button onclick='postPlayer(" + String(STAT_PLAY) + ")'>Play</button><br>"
  "        <br>"
  "        <button onclick='postPlayer(" + String(STAT_PAUSE) + ")'>Pause</button><br>"
  "        <br>"
  "        <button onclick='postPlayer(" + String(STAT_STOPPING) + ")'>Stop</button><br>"
  "</div>"
  ;

void handlePlayer(){
  server.send(200,"text/html",rootPlayerHTML);
}


String rootMmlHeaderHTML = ""
  "<div>"
  "       <h1>MML Editor</h1>"
  "       <div id='timbre_player'>"
  "               SKIP&nbsp;<input id='skip' type='number' min='0' max='500000' value='0'>"
  "               <button onclick='postPlayer(2)'>Play</button>"
  "               <button onclick='postPlayer(1)'>Stop</button>"
  "       </div>"
  "       <br>"
  "       <br>"
  "       <div>"
  "               <button onclick='postMml()'>Upload</button>"
  "       </div>"
  ;
String rootMmlTrackPart1HTML = ""
  "       <div class='track'>"
  "               <label>Track"
  ;
String rootMmlTrackPart2HTML = ""
  "</label><br>"
  "               <textarea id='track"
  ;
String rootMmlTrackPart3HTML = ""
  "'>"
  ;
String rootMmlTrackPart4HTML = ""
  "</textarea>"
  "       </div>"
  ;
String rootMmlFooterHTML = ""
  "</div>"
  ;
  
void waitPlayer(){
  playStatus = STAT_STOPPING;
  while(playStatus != STAT_STOP){
    delay(100);
  }
}

void handleMml(){
  String data = "";
  
  waitPlayer();
  
  data = rootMmlHeaderHTML;
  for(int i=1;i <= CHANNELS;i++){
    String number = "0" + String(i);
    number = number.substring(number.length() - 2);
    File file = LittleFS.open("track" + number + ".mml", "r");
    String mml = "";
    if(file){
      mml = file.readString();
    }
    file.close();
    
    data = data + rootMmlTrackPart1HTML + number + rootMmlTrackPart2HTML + number + rootMmlTrackPart3HTML + mml + rootMmlTrackPart4HTML;
  }
  data = data + rootMmlFooterHTML;
  server.send(200,"text/html",data);
}

String rootTimbreHeaderHTML = ""
"<div>"
"       <h1>Timbre Editor</h1>"
"       <div>"
;

String rootTimbrePlayerHeaderHTML = ""
"               <div id='timbre_player'>"
"                       Track"
"                       <select id='track'>"
;
String rootTimbrePlayerPart1HTML = ""
"                               <option value='"
;
String rootTimbrePlayerPart2HTML = ""
"'>"
;
String rootTimbrePlayerPart3HTML = ""
"</option>"
;
String rootTimbrePlayerFooterHTML = ""
"                       </select>"
"                       <button onclick='postPlayer(" + String(STAT_PLAY) + ")'>Play</button>"
"                       <button onclick='postPlayer(" + String(STAT_STOPPING) + ")'>Stop</button>"
"               </div>"
"               <br>"
"               <br>"
;
String rootTimbreSelectHeaderHTML = ""
"               Timbre@"
"                <select id='timbre' onchange='onSelectTimbre()'>"
;
String rootTimbreSelectPart1HTML = ""
"                        <option value='"
;
String rootTimbreSelectPart2HTML = ""
"'>"
;
String rootTimbreSelectPart2SelectedHTML = ""
"' selected>"
;
String rootTimbreSelectPart3HTML = ""
"</option>"
;
String rootTimbreSelectFooter1HTML = ""
"                </select>"
"               <button onclick='postTimbre(1)'>Upload</button>"
"               <br>"
"               <br>"
"               <label>Description</label>"
"               <input id='description' type='text' value='"
;
String rootTimbreSelectFooter2HTML = ""
"'>"
"       </div>"
;

String rootTimbreCarrierHeaderHTML = ""
"       <fieldset class='timbre'>"
"               <legend>Carrier</legend>"
"               <label>Waveform</label>"
"               <select id='waveform_type' onchange='postTimbre(0)'>"
;
String rootTimbreCarrierSelectPart1HTML = ""
"                       <option value='"
;
String rootTimbreCarrierSelectPart2HTML = ""
"'>"
;
String rootTimbreCarrierSelectPart2SelectedHTML = ""
"' selected>"
;
String rootTimbreCarrierSelectPart3HTML = ""
"</option>"
;
String rootTimbreCarrierSelectFooterHTML = ""
"               </select>"
;

String rootTimbreCarrierEnvelopeHeaderHTML = ""
"               <label>Enable Envelope</label>"
"               <select id='enable_envelope' onchange='postTimbre(0)'>"
"                       <option value='0'"
;
String rootTimbreCarrierEnvelopePart1HTML = ""
">False</option>"
"                       <option value='1'"
;
String rootTimbreCarrierEnvelopePart2HTML = ""
">True</option>"
"               </select>"
;

String rootTimbreCarrierAttackHeaderHTML = ""
"               <label>Attack</label>"
"               <input id='ticks_attack' type='number' min='1' max='10000' onchange='onInputChange(event)' value='"
;
String rootTimbreCarrierAttackPart1HTML = ""
"'>"
"               <input id='ticks_attack_slider' class='timbre_span' type='range' min='1' max='5000' onchange='onSliderChange(event)' value='"
;
String rootTimbreCarrierAttackFooterHTML = ""
"'>"
;

String rootTimbreCarrierDecayHeaderHTML = ""
"               <label>Decay</label>"
"               <input id='ticks_decay' type='number' min='1' max='10000' onchange='onInputChange(event)' value='"
;
String rootTimbreCarrierDecayPart1HTML = ""
"'>"
"               <input id='ticks_decay_slider' class='timbre_span' type='range' min='1' max='5000' onchange='onSliderChange(event)' value='"
;
String rootTimbreCarrierDecayFooterHTML = ""
"'>"
;

String rootTimbreCarrierSustainHeaderHTML = ""
"               <label>Sustain</label>"
"               <input id='volume_sustain' type='number' min='1' max='100' onchange='onInputChange(event)' value='"
;
String rootTimbreCarrierSustainPart1HTML = ""
"'>"
"               <input id='volume_sustain_slider' class='timbre_span' type='range' min='1' max='100' onchange='onSliderChange(event)' value='"
;
String rootTimbreCarrierSustainFooterHTML = ""
"'>"
;

String rootTimbreCarrierReleaseHeaderHTML = ""
"               <label>Release</label>"
"               <input id='ticks_release' type='number' min='1' max='10000' onchange='onInputChange(event)' value='"
;
String rootTimbreCarrierReleasePart1HTML = ""
"'>"
"               <input id='ticks_release_slider' class='timbre_span' type='range' min='1' max='5000' onchange='onSliderChange(event)' value='"
;
String rootTimbreCarrierReleaseFooterHTML = ""
"'>"
"       </fieldset>"
;

String rootTimbreModulatorHeaderHTML = ""
"       <fieldset class='timbre'>"
"               <legend>Modulator</legend>"
"               <label>Enable Modulator</label>"
"               <select id='enable_modulator' onchange='postTimbre(0)'>"
;
String rootTimbreModulatorEnablePart1HTML = ""
"                       <option value='0'"
;
String rootTimbreModulatorEnablePart2HTML = ""
">False</option>"
"                       <option value='1'"
;
String rootTimbreModulatorEnablePart3HTML = ""
">True</option>"
"               </select>"
;

String rootTimbreModulatorTypeHeaderHTML = ""
"               <label>Type</label>"
"               <select id='type_modulator' onchange='postTimbre(0)'>"
"                       <option value='0'"
;
String rootTimbreModulatorTypePart1HTML = ""
">Type 0</option>"
"                       <option value='1'"
;
String rootTimbreModulatorTypePart2HTML = ""
">Type 1</option>"
"               </select>"
;

String rootTimbreModulatorWaveformHeaderHTML = ""
"               <label>Waveform</label>"
"               <select id='waveform_type_modulator' onchange='postTimbre(0)'>"
;
String rootTimbreModulatorWaveformPart1HTML = ""
"                       <option value='"
;
String rootTimbreModulatorWaveformPart2HTML = ""
"'>"
;
String rootTimbreModulatorWaveformPart2SelectedHTML = ""
"' selected>"
;
String rootTimbreModulatorWaveformPart3HTML = ""
"</option>"
;
String rootTimbreModulatorWaveformFooterHTML = ""
"               </select>"
;

String rootTimbreModulatorVolumeHeaderHTML = ""
"               <label>Volume</label>"
"               <input id='base_volume_modulator' type='number' min='1' max='100' onchange='onInputChange(event)' value='"
;
String rootTimbreModulatorVolumePart1HTML = ""
"'>"
"               <input id='base_volume_modulator_slider' class='timbre_span' type='range' min='1' max='100' onchange='onSliderChange(event)' value='"
;
String rootTimbreModulatorVolumePart2HTML = ""
"'>"
;

String rootTimbreModulatorFrequencyHeaderHTML = ""
"               <label>Frequency Ratio</label>"
"               <input id='frequency_ratio' type='number' min='0.001' max='15' step='0.001' onchange='onInputChangeFRQ(event)' value='"
;
String rootTimbreModulatorFrequencyPart1HTML = ""
"'>"
"               <input id='frequency_ratio_high_slider' class='timbre_span' type='range' min='0' max='15' onchange='onSliderChangeFRQ()' value='"
;
String rootTimbreModulatorFrequencyPart2HTML = ""
"'>"
"               <input id='frequency_ratio_low_slider' class='timbre_span' type='range' min='0' max='999' onchange='onSliderChangeFRQ()' value='"
;
String rootTimbreModulatorFrequencyPart3HTML = ""
"'>"
;

String rootTimbreModulatorEnvelopeHeaderHTML = ""
"               <label>Enable Envelope</label>"
"               <select id='enable_envelope_modulator' onchange='postTimbre(0)'>"
"                       <option value='0'"
;
String rootTimbreModulatorEnvelopePart1HTML = ""
">False</option>"
"                       <option value='1'"
;
String rootTimbreModulatorEnvelopePart2HTML = ""
">True</option>"
"               </select>"
;

String rootTimbreModulatorAttackHeaderHTML = ""
"               <label>Attack</label>"
"               <input id='ticks_attack_modulator' type='number' min='1' max='10000' onchange='onInputChange(event)' value='"
;
String rootTimbreModulatorAttackPart1HTML = ""
"'>"
"               <input id='ticks_attack_modulator_slider' class='timbre_span' type='range' min='1' max='5000' onchange='onSliderChange(event)' value='"
;
String rootTimbreModulatorAttackFooterHTML = ""
"'>"
;

String rootTimbreModulatorDecayHeaderHTML = ""
"               <label>Decay</label>"
"               <input id='ticks_decay_modulator' type='number' min='1' max='10000' onchange='onInputChange(event)' value='"
;
String rootTimbreModulatorDecayPart1HTML = ""
"'>"
"               <input id='ticks_decay_modulator_slider' class='timbre_span' type='range' min='1' max='5000' onchange='onSliderChange(event)' value='"
;
String rootTimbreModulatorDecayFooterHTML = ""
"'>"
;

String rootTimbreModulatorSustainHeaderHTML = ""
"               <label>Sustain</label>"
"               <input id='volume_sustain_modulator' type='number' min='1' max='100' onchange='onInputChange(event)' value='"
;
String rootTimbreModulatorSustainPart1HTML = ""
"'>"
"               <input id='volume_sustain_modulator_slider' class='timbre_span' type='range' min='1' max='100' onchange='onSliderChange(event)' value='"
;
String rootTimbreModulatorSustainFooterHTML = ""
"'>"
;

String rootTimbreModulatorReleaseHeaderHTML = ""
"               <label>Release</label>"
"               <input id='ticks_release_modulator' type='number' min='1' max='10000' onchange='onInputChange(event)' value='"
;
String rootTimbreModulatorReleasePart1HTML = ""
"'>"
"               <input id='ticks_release_modulator_slider' class='timbre_span' type='range' min='1' max='5000' onchange='onSliderChange(event)' value='"
;
String rootTimbreModulatorReleaseFooterHTML = ""
"'>"
"       </fieledset>"
;

String rootTimbreFooterHTML = ""
  "</div>"
  ;

String getWaveformText(int i){
  String ret = "";
  
  switch(i){
    case SINE_WAVE:
      ret = "SINE";
      break;
    case SQUARE_WAVE:
      ret = "SQUARE";
      break;
    case TRIANGLE_WAVE:
      ret = "TRIANGLE";
      break;
    case SAWTOOTH_WAVE:
      ret = "SAWTOOTH";
      break;
    case NOISE_WAVE:
      ret = "NOISE";
      break;
    default:
      ret = "SINE";
      break;
  }
  
  return ret;
}

void initializeTmpTimbre(){
  
  tmpTimbreData.description = "";
  tmpTimbreData.waveform_type = SINE_WAVE;
  tmpTimbreData.enable_envelope = true;
  tmpTimbreData.ticks_attack = DEFAULT_TICKS_ATTACK;
  tmpTimbreData.ticks_decay = DEFAULT_TICKS_DECAY;
  tmpTimbreData.volume_sustain = DEFAULT_VOLUME_SUSTAIN;
  tmpTimbreData.ticks_release = DEFAULT_TICKS_RELEASE;
  tmpTimbreData.enable_modulator = false;
  tmpTimbreData.type_modulator = DEFAULT_MODULATION_TYPE;
  tmpTimbreData.waveform_type_modulator = SINE_WAVE;
  tmpTimbreData.base_volume_modulator = DEFAULT_BASE_VOLUME_MODULATOR;
  tmpTimbreData.frequency_ratio = DEFAULT_FREQUENCY_RATIO;
  tmpTimbreData.enable_envelope_modulator = false;
  tmpTimbreData.ticks_attack_modulator = DEFAULT_TICKS_ATTACK;
  tmpTimbreData.ticks_decay_modulator = DEFAULT_TICKS_DECAY;
  tmpTimbreData.volume_sustain_modulator = DEFAULT_VOLUME_SUSTAIN;
  tmpTimbreData.ticks_release_modulator = DEFAULT_TICKS_RELEASE;
  
}

void handleTimbre(){
  String data = "";
  
  if(trackPlayIdx == CHANNELS){
    waitPlayer();
    trackPlayIdx = 0;
  }
  
  uint32_t timbre = server.arg("timbre").toInt();
  if(timbre >= TIMBRES){
    timbre = 0;
  }
  
  initializeTmpTimbre();
  
  String temp = "0" + String(timbre);
  String number = temp.substring(temp.length() - 2);
  String filename = "timbre" + number + ".dat";
  
  File file = LittleFS.open(filename, "r");
  
  if(file){
    //Serial.println(filename + ":" + String(file.available()));
    
    while(file.available()){
      String line = file.readStringUntil('\n');
      line.trim();
      
      String key;
      String value;
      int separator_index = line.indexOf('=');
      if(separator_index != -1){
        key = line.substring(0 ,separator_index);
        value = line.substring(separator_index + 1);
      }
      
      if(key == "WaveformType"){
        int type = value.toInt();
        if(0 <= type && type < WAVEFORMS){
          tmpTimbreData.waveform_type = type;
        }
      }else if(key == "EnableEnvelope"){
        int envelope = value.toInt();
        if(envelope == 0){
          tmpTimbreData.enable_envelope = false;
        }else{
          tmpTimbreData.enable_envelope = true;
        }
      }else if(key == "TicksAttack"){
        int attack = value.toInt();
        if(attack != 0){
          tmpTimbreData.ticks_attack = attack;
        }
      }else if(key == "TicksDecay"){
        int decay = value.toInt();
        if(decay != 0){
          tmpTimbreData.ticks_decay = decay;
        }
      }else if(key == "VolumeSustain"){
        tmpTimbreData.volume_sustain = value.toInt();
      }else if(key == "TicksRelease"){
        int release_ = value.toInt();
        if(release_ != 0){
          tmpTimbreData.ticks_release = release_;
        }
      }else if(key == "EnableModulator"){
        int enable = value.toInt();
        if(enable == 0){
          tmpTimbreData.enable_modulator = false;
        }else{
          tmpTimbreData.enable_modulator = true;
        }
      }else if(key == "TypeModulator"){
        tmpTimbreData.type_modulator = value.toInt();
      }else if(key == "VolumeModulator"){
        tmpTimbreData.base_volume_modulator = value.toInt();
      }else if(key == "WaveformTypeModulator"){
        int type = value.toInt();
        if(0 <= type && type < WAVEFORMS){
          tmpTimbreData.waveform_type_modulator = value.toInt();
        }
      }else if(key == "FrequencyRatio"){
        tmpTimbreData.frequency_ratio = value.toFloat();
      }else if(key == "EnableEnvelopeModulator"){
        int envelope = value.toInt();
        if(envelope == 0){
          tmpTimbreData.enable_envelope_modulator = false;
        }else{
          tmpTimbreData.enable_envelope_modulator = true;
        }
      }else if(key == "TicksAttackModulator"){
        int attack = value.toInt();
        if(attack != 0){
          tmpTimbreData.ticks_attack_modulator = attack;
        }
      }else if(key == "TicksDecayModulator"){
        int decay = value.toInt();
        if(decay != 0){
          tmpTimbreData.ticks_decay_modulator = decay;
        }
      }else if(key == "VolumeSustainModulator"){
        tmpTimbreData.volume_sustain_modulator = value.toInt();
      }else if(key == "TicksReleaseModulator"){
        int release_ = value.toInt();
        if(release_ != 0){
          tmpTimbreData.ticks_release_modulator = release_;
        }
      }else if(key == "Description"){
        tmpTimbreData.description = value;
      }
    }
    
    file.close();
    /*
    Serial.println("Description             :" + String(tmpTimbreData.description));
    Serial.println("WaveformType            :" + String(tmpTimbreData.waveform_type));
    Serial.println("EnableEnvelope          :" + String(tmpTimbreData.enable_envelope));
    Serial.println("TicksAttack             :" + String(tmpTimbreData.ticks_attack));
    Serial.println("TicksDecay              :" + String(tmpTimbreData.ticks_decay));
    Serial.println("VolumeSustain           :" + String(tmpTimbreData.volume_sustain));
    Serial.println("TicksRelease            :" + String(tmpTimbreData.ticks_release));
    Serial.println("EnableModulator         :" + String(tmpTimbreData.enable_modulator));
    Serial.println("TypeModulator           :" + String(tmpTimbreData.type_modulator));
    Serial.println("VolumeModulator         :" + String(tmpTimbreData.base_volume_modulator));
    Serial.println("WaveformTypeModulator   :" + String(tmpTimbreData.waveform_type_modulator));
    Serial.println("FrequencyRatio          :" + String(tmpTimbreData.frequency_ratio,3));
    Serial.println("EnableEnvelopeModulator :" + String(tmpTimbreData.enable_envelope_modulator));
    Serial.println("TicksAttackModulator    :" + String(tmpTimbreData.ticks_attack_modulator));
    Serial.println("TicksDecayModulator     :" + String(tmpTimbreData.ticks_decay_modulator));
    Serial.println("VolumeSustainModulator  :" + String(tmpTimbreData.volume_sustain_modulator));
    Serial.println("TicksReleaseModulator   :" + String(tmpTimbreData.ticks_release_modulator));
    */
  }else{
    //Serial.println(filename + ":Failed to open file");
  }
  
  if(trackPlayIdx < CHANNELS){
    updateTrackTimbre(trackPlayIdx);
  }
  
  data = rootTimbreHeaderHTML;
  
  
  data = data + rootTimbrePlayerHeaderHTML;
  for(int i=0;i < CHANNELS;i++){
    data = data + rootTimbrePlayerPart1HTML + String(i) + rootTimbrePlayerPart2HTML + String(i+1) + rootTimbrePlayerPart3HTML;
  }
  data = data + rootTimbrePlayerFooterHTML;
  
  
  data = data + rootTimbreSelectHeaderHTML;
  for(int i=0;i < TIMBRES;i++){
    if(i == timbre){
      data = data + rootTimbreSelectPart1HTML + String(i) + rootTimbreSelectPart2SelectedHTML + String(i) + rootTimbreSelectPart3HTML;      
    }else{
      data = data + rootTimbreSelectPart1HTML + String(i) + rootTimbreSelectPart2HTML + String(i) + rootTimbreSelectPart3HTML;
    }
  }
  data = data + rootTimbreSelectFooter1HTML + tmpTimbreData.description + rootTimbreSelectFooter2HTML;
  
  
  data = data + rootTimbreCarrierHeaderHTML;
  for(int i=0;i < WAVEFORMS;i++){
    if(i == tmpTimbreData.waveform_type){
      data = data + rootTimbreCarrierSelectPart1HTML + String(i) + rootTimbreCarrierSelectPart2SelectedHTML + getWaveformText(i) + rootTimbreCarrierSelectPart3HTML;      
    }else{
      data = data + rootTimbreCarrierSelectPart1HTML + String(i) + rootTimbreCarrierSelectPart2HTML + getWaveformText(i) + rootTimbreCarrierSelectPart3HTML;
    }
  }
  data = data + rootTimbreCarrierSelectFooterHTML;
  String txt0;
  String txt1;
  if(tmpTimbreData.enable_envelope){
    txt0 = "";
    txt1 = " selected";
  }else{
    txt0 = " selected";
    txt1 = "";
  }
  data = data + rootTimbreCarrierEnvelopeHeaderHTML + txt0 + rootTimbreCarrierEnvelopePart1HTML + txt1 + rootTimbreCarrierEnvelopePart2HTML;
  data = data + rootTimbreCarrierAttackHeaderHTML + String(tmpTimbreData.ticks_attack) + rootTimbreCarrierAttackPart1HTML + String(tmpTimbreData.ticks_attack) + rootTimbreCarrierAttackFooterHTML;
  data = data + rootTimbreCarrierDecayHeaderHTML + String(tmpTimbreData.ticks_decay) + rootTimbreCarrierDecayPart1HTML + String(tmpTimbreData.ticks_decay) + rootTimbreCarrierDecayFooterHTML;
  data = data + rootTimbreCarrierSustainHeaderHTML + String(tmpTimbreData.volume_sustain) + rootTimbreCarrierSustainPart1HTML + String(tmpTimbreData.volume_sustain) + rootTimbreCarrierSustainFooterHTML;
  data = data + rootTimbreCarrierReleaseHeaderHTML + String(tmpTimbreData.ticks_release) + rootTimbreCarrierReleasePart1HTML + String(tmpTimbreData.ticks_release) + rootTimbreCarrierReleaseFooterHTML;
  
  
  data = data + rootTimbreModulatorHeaderHTML;
  if(tmpTimbreData.enable_modulator){
    txt0 = "";
    txt1 = " selected";
  }else{
    txt0 = " selected";
    txt1 = "";
  }
  data = data + rootTimbreModulatorEnablePart1HTML + txt0 + rootTimbreModulatorEnablePart2HTML + txt1 + rootTimbreModulatorEnablePart3HTML;
  if(tmpTimbreData.type_modulator == 1){
    txt0 = "";
    txt1 = " selected";
  }else{
    txt0 = " selected";
    txt1 = "";
  }
  data = data + rootTimbreModulatorTypeHeaderHTML + txt0 + rootTimbreModulatorTypePart1HTML + txt1 + rootTimbreModulatorTypePart2HTML;
  data = data + rootTimbreModulatorWaveformHeaderHTML;
  for(int i=0;i < WAVEFORMS;i++){
    if(i == tmpTimbreData.waveform_type_modulator){
      data = data + rootTimbreModulatorWaveformPart1HTML + String(i) + rootTimbreModulatorWaveformPart2SelectedHTML + getWaveformText(i) + rootTimbreModulatorWaveformPart3HTML;      
    }else{
      data = data + rootTimbreModulatorWaveformPart1HTML + String(i) + rootTimbreModulatorWaveformPart2HTML + getWaveformText(i) + rootTimbreModulatorWaveformPart3HTML;
    }
  }
  data = data + rootTimbreModulatorWaveformFooterHTML;
  data = data + rootTimbreModulatorVolumeHeaderHTML + String(tmpTimbreData.base_volume_modulator) + rootTimbreModulatorVolumePart1HTML + String(tmpTimbreData.base_volume_modulator) + rootTimbreModulatorVolumePart2HTML;
  txt0 = String(int(tmpTimbreData.frequency_ratio));
  txt1 = String(tmpTimbreData.frequency_ratio,3).substring(String(tmpTimbreData.frequency_ratio,3).indexOf('.') + 1);
  data = data + rootTimbreModulatorFrequencyHeaderHTML + String(tmpTimbreData.frequency_ratio,3) + rootTimbreModulatorFrequencyPart1HTML + txt0 + rootTimbreModulatorFrequencyPart2HTML + txt1 + rootTimbreModulatorFrequencyPart3HTML;
  if(tmpTimbreData.enable_envelope_modulator){
    txt0 = "";
    txt1 = " selected";
  }else{
    txt0 = " selected";
    txt1 = "";
  }
  data = data + rootTimbreModulatorEnvelopeHeaderHTML + txt0 + rootTimbreModulatorEnvelopePart1HTML + txt1 + rootTimbreModulatorEnvelopePart2HTML;
  data = data + rootTimbreModulatorAttackHeaderHTML + String(tmpTimbreData.ticks_attack_modulator) + rootTimbreModulatorAttackPart1HTML + String(tmpTimbreData.ticks_attack_modulator) + rootTimbreModulatorAttackFooterHTML;
  data = data + rootTimbreModulatorDecayHeaderHTML + String(tmpTimbreData.ticks_decay_modulator) + rootTimbreModulatorDecayPart1HTML + String(tmpTimbreData.ticks_decay_modulator) + rootTimbreModulatorDecayFooterHTML;
  data = data + rootTimbreModulatorSustainHeaderHTML + String(tmpTimbreData.volume_sustain_modulator) + rootTimbreModulatorSustainPart1HTML + String(tmpTimbreData.volume_sustain_modulator) + rootTimbreModulatorSustainFooterHTML;
  data = data + rootTimbreModulatorReleaseHeaderHTML + String(tmpTimbreData.ticks_release_modulator) + rootTimbreModulatorReleasePart1HTML + String(tmpTimbreData.ticks_release_modulator) + rootTimbreModulatorReleaseFooterHTML;
  
  
  data = data + rootTimbreFooterHTML;
  
  server.send(200,"text/html",data);
}

void getArgTimbre(){
  uint32_t value;
  
  tmpTimbreData.description = server.arg("description");
  value = server.arg("waveform_type").toInt();
  if(value < WAVEFORMS){
    tmpTimbreData.waveform_type = value;
  }
  value = server.arg("enable_envelope").toInt();
  if(value == 1){
    tmpTimbreData.enable_envelope = true;
  }else{
    tmpTimbreData.enable_envelope = false;
  }
  value = server.arg("ticks_attack").toInt();
  if(0 < value && value <= UINT16_MAX){
    tmpTimbreData.ticks_attack = value;
  }
  value = server.arg("ticks_decay").toInt();
  if(0 < value && value <= UINT16_MAX){
    tmpTimbreData.ticks_decay = value;
  }
  value = server.arg("volume_sustain").toInt();
  if(value <= VOLUME_MAX){
    tmpTimbreData.volume_sustain = value;
  }
  value = server.arg("ticks_release").toInt();
  if(0 < value && value <= UINT16_MAX){
    tmpTimbreData.ticks_release = value;
  }
  value = server.arg("enable_modulator").toInt();
  if(value == 1){
    tmpTimbreData.enable_modulator = true;
  }else{
    tmpTimbreData.enable_modulator = false;
  }
  value = server.arg("type_modulator").toInt();
  if(value == 1){
    tmpTimbreData.type_modulator = value;
  }else{
    tmpTimbreData.type_modulator = DEFAULT_MODULATION_TYPE;
  }
  value = server.arg("waveform_type_modulator").toInt();
  if(value < WAVEFORMS){
    tmpTimbreData.waveform_type_modulator = value;
  }
  value = server.arg("base_volume_modulator").toInt();
  if(value <= VOLUME_MAX){
    tmpTimbreData.base_volume_modulator = value;
  }
  float frq = server.arg("frequency_ratio").toFloat();
  if(0 <= frq && frq <= 15.0){
    tmpTimbreData.frequency_ratio = frq;
  }
  value = server.arg("enable_envelope_modulator").toInt();
  if(value == 1){
    tmpTimbreData.enable_envelope_modulator = true;
  }else{
    tmpTimbreData.enable_envelope_modulator = false;
  }
  value = server.arg("ticks_attack_modulator").toInt();
  if(0 < value && value <= UINT16_MAX){
    tmpTimbreData.ticks_attack_modulator = value;
  }
  value = server.arg("ticks_decay_modulator").toInt();
  if(0 < value && value <= UINT16_MAX){
    tmpTimbreData.ticks_decay_modulator = value;
  }
  value = server.arg("volume_sustain_modulator").toInt();
  if(0 < value && value <= UINT16_MAX){
    tmpTimbreData.volume_sustain_modulator = value;
  }
  value = server.arg("ticks_release_modulator").toInt();
  if(0 < value && value <= UINT16_MAX){
    tmpTimbreData.ticks_release_modulator = value;
  }
  
  /*
  Serial.println("Description             :" + String(tmpTimbreData.description));
  Serial.println("WaveformType            :" + String(tmpTimbreData.waveform_type));
  Serial.println("EnableEnvelope          :" + String(tmpTimbreData.enable_envelope));
  Serial.println("TicksAttack             :" + String(tmpTimbreData.ticks_attack));
  Serial.println("TicksDecay              :" + String(tmpTimbreData.ticks_decay));
  Serial.println("VolumeSustain           :" + String(tmpTimbreData.volume_sustain));
  Serial.println("TicksRelease            :" + String(tmpTimbreData.ticks_release));
  Serial.println("EnableModulator         :" + String(tmpTimbreData.enable_modulator));
  Serial.println("TypeModulator           :" + String(tmpTimbreData.type_modulator));
  Serial.println("VolumeModulator         :" + String(tmpTimbreData.base_volume_modulator));
  Serial.println("WaveformTypeModulator   :" + String(tmpTimbreData.waveform_type_modulator));
  Serial.println("FrequencyRatio          :" + String(tmpTimbreData.frequency_ratio,3));
  Serial.println("EnableEnvelopeModulator :" + String(tmpTimbreData.enable_envelope_modulator));
  Serial.println("TicksAttackModulator    :" + String(tmpTimbreData.ticks_attack_modulator));
  Serial.println("TicksDecayModulator     :" + String(tmpTimbreData.ticks_decay_modulator));
  Serial.println("VolumeSustainModulator  :" + String(tmpTimbreData.volume_sustain_modulator));
  Serial.println("TicksReleaseModulator   :" + String(tmpTimbreData.ticks_release_modulator));
  */

}

void handleAPI(){
  String method_ = server.arg("method");
  
  if(method_ == "setPlayer"){
    uint32_t status_ = server.arg("status").toInt();
    if(STAT_STOP < status_ && status_ <= STAT_PAUSE){
      uint32_t trackPlayIdxOld = trackPlayIdx;
      
      trackPlayIdx = CHANNELS;
      String key = "track";
      if(server.hasArg(key)){
        uint32_t value = server.arg(key).toInt();
        if(value < CHANNELS){
          trackPlayIdx = value;
          initializeTmpTimbre();
          getArgTimbre();
        }
      }
      
      key = "skip";
      if(server.hasArg(key) && status_ == STAT_PLAY){
        uint32_t value = server.arg(key).toInt();
        if(value < 500000){
          skipTicks = value * BEAT_TICKS;
        }
      }
      
      if(status_ == STAT_PLAY && trackPlayIdxOld != trackPlayIdx){
        waitPlayer();
      }
      playStatus = status_;
    }
  }else if(method_ == "setMml"){
    waitPlayer();
    
    for(int i=1;i <= CHANNELS;i++){
      String number = "0" + String(i);
      number = number.substring(number.length() - 2);
      String key = "track" + number;
      if(server.hasArg(key)){
        String value = server.arg(key);
        
        File file = LittleFS.open(key + ".mml", "w");
        if(file){
          file.print(value);
        }
        file.close();
      }
    }
  }else if(method_ == "setTimbre"){
    initializeTmpTimbre();
    getArgTimbre();
    
    if(server.hasArg("track")){
      uint32_t index = server.arg("track").toInt();
      if(index < CHANNELS){
        updateTrackTimbre(index);
      }
    }else if(server.arg("upload") == "1" && server.hasArg("timbre")){
      
      uint32_t timbre = server.arg("timbre").toInt();
      if(timbre < TIMBRES){
        String temp = "0" + String(timbre);
        String number = temp.substring(temp.length() - 2);
        String filename = "timbre" + number + ".dat";
        
        File file = LittleFS.open(filename, "w");
        if(file){
          file.print("Description=" + String(tmpTimbreData.description) + '\n');
          file.print("WaveformType=" + String(tmpTimbreData.waveform_type) + '\n');
          file.print("EnableEnvelope=" + String(tmpTimbreData.enable_envelope) + '\n');
          file.print("TicksAttack=" + String(tmpTimbreData.ticks_attack) + '\n');
          file.print("TicksDecay=" + String(tmpTimbreData.ticks_decay) + '\n');
          file.print("VolumeSustain=" + String(tmpTimbreData.volume_sustain) + '\n');
          file.print("TicksRelease=" + String(tmpTimbreData.ticks_release) + '\n');
          file.print("EnableModulator=" + String(tmpTimbreData.enable_modulator) + '\n');
          file.print("TypeModulator=" + String(tmpTimbreData.type_modulator) + '\n');
          file.print("VolumeModulator=" + String(tmpTimbreData.base_volume_modulator) + '\n');
          file.print("WaveformTypeModulator=" + String(tmpTimbreData.waveform_type_modulator) + '\n');
          file.print("FrequencyRatio=" + String(tmpTimbreData.frequency_ratio,3) + '\n');
          file.print("EnableEnvelopeModulator=" + String(tmpTimbreData.enable_envelope_modulator) + '\n');
          file.print("TicksAttackModulator=" + String(tmpTimbreData.ticks_attack_modulator) + '\n');
          file.print("TicksDecayModulator=" + String(tmpTimbreData.ticks_decay_modulator) + '\n');
          file.print("VolumeSustainModulator=" + String(tmpTimbreData.volume_sustain_modulator) + '\n');
          file.print("TicksReleaseModulator=" + String(tmpTimbreData.ticks_release_modulator) + '\n');
        }
        file.close();
      }
    }
  }
  
  server.send(200, "text/plain", "Data received successfully");
}

void connectingWiFi(){
  if(WiFi.status() != WL_CONNECTED){
    for(int i=0;i<5;i++){
      WiFi.begin(SSID_TEXT, PASS_TEXT);
      if(WiFi.status() == WL_CONNECTED){
        Serial.println("Connected to WiFi");
        break;
      }
      Serial.println("Connecting to WiFi...");
      delay(1000);
    }
  }
}

inline bool isEven(uint16_t value){
  
  if((value >> 1) << 1 == value){
    return true;
  }
  
  return false;
}

inline uint16_t calculatePhaseModulation(uint16_t channel_index, uint16_t carrier_phase, int16_t modulator_amplitude){
  int16_t phase;
  int16_t carrier_value;
  
  if(trackData[channel_index].type_modulator == DEFAULT_MODULATION_TYPE){
    phase = carrier_phase + modulator_amplitude;  // 0~63,-64~64
    if(phase < 0){
      phase = SAMPLE + phase; 
    }else if(phase >= SAMPLE){
      phase = phase - SAMPLE;
    }
  }else{
    carrier_value = map(carrier_phase, 0, SAMPLE -1, -SAMPLE, SAMPLE -1);
    phase = map(carrier_value + modulator_amplitude, -MAX_AMPLITUDE - SAMPLE, MAX_AMPLITUDE + (SAMPLE - 1), 0, 63);
  }
  
  return phase;
}

uint16_t default_level = MAX_AMPLITUDE * CHANNELS;
uint16_t pwmPin;
uint16_t sliceNum;
void setPwmLevel(){
  
  for(int i=0;i<CHANNELS;i++){
    if(trackData[i].waveform_type == NOISE_WAVE){
      phaseCarrier[i] += random(UINT16_MAX);
    }else{
      phaseCarrier[i] += trackData[i].frequency;
    }
    if(trackData[i].waveform_type_modulator == NOISE_WAVE){
      phaseModulator[i] += random(trackData[i].frequency_modulator);
    }else{
      phaseModulator[i] += trackData[i].frequency_modulator;
    }
  }
  
  uint16_t level = default_level;
  if(playStatus == STAT_PLAY && skipTicks == 0){
    for(int i=0;i<CHANNELS;i++){
      if(trackData[i].enable){
        if(trackData[i].enable_modulator){
          level += calculateAmplitudeTable[waveForm[trackData[i].waveform_type][calculatePhaseModulation(i,phaseCarrier[i] >> 10, calculateAmplitudeTable[waveForm[trackData[i].waveform_type_modulator][phaseModulator[i] >> 10] + + MAX_AMPLITUDE][trackData[i].volume_modulator])] + MAX_AMPLITUDE][trackData[i].volume];
        }else{
          level += calculateAmplitudeTable[waveForm[trackData[i].waveform_type][phaseCarrier[i] >> 10] + MAX_AMPLITUDE][trackData[i].volume];
        }
      }
    }
  }
  
  if(isEven(pwmPin)){
    pwm_set_chan_level(sliceNum, PWM_CHAN_A, level);
  }else{
    pwm_set_chan_level(sliceNum, PWM_CHAN_B, level);
  }
  
  pwm_clear_irq(sliceNum);
}

void setup() {
  Serial.begin(115200);
  delay(3000);
  
  connectingWiFi();
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  
  pwmPin = 0;
  
  gpio_set_function(pwmPin, GPIO_FUNC_PWM);
  
  sliceNum = pwm_gpio_to_slice_num(pwmPin);
  
  pwm_set_irq_enabled(sliceNum, true);
  
  irq_set_exclusive_handler(PWM_IRQ_WRAP, setPwmLevel);
  irq_set_enabled(PWM_IRQ_WRAP, true);
  
  //pwm_set_clkdiv(sliceNum, 7.8125);  // 16 Mhz(128 / 7.8125)
  //pwm_set_wrap(sliceNum, 256); // 62500 Hz
  //pwm_set_clkdiv(sliceNum, 4.0);  // 32 Mhz(128 / 4.0)
  //pwm_set_wrap(sliceNum, 512); // 62500 Hz
  //pwm_set_clkdiv(sliceNum, 2.0);  // 64 Mhz(128 / 2.0)
  //pwm_set_wrap(sliceNum, 1024); // 62500 Hz
  pwm_set_clkdiv(sliceNum, 1.0); // 128 Mhz(128 / 1.0)
  pwm_set_wrap(sliceNum, 2048);  // 62500 Hz
  
  if(isEven(pwmPin)){
    pwm_set_chan_level(sliceNum, PWM_CHAN_A, 0);
  }else{
    pwm_set_chan_level(sliceNum, PWM_CHAN_B, 0);
  }
  
  pwm_set_enabled(sliceNum, true);
  
  finishSetup = true;
}

void loop(){
  delay(25);
  server.handleClient();
  connectingWiFi();
}
