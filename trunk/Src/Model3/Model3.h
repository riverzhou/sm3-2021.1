/**
 ** Supermodel
 ** A Sega Model 3 Arcade Emulator.
 ** Copyright 2011 Bart Trzynadlowski, Nik Henson 
 **
 ** This file is part of Supermodel.
 **
 ** Supermodel is free software: you can redistribute it and/or modify it under
 ** the terms of the GNU General Public License as published by the Free 
 ** Software Foundation, either version 3 of the License, or (at your option)
 ** any later version.
 **
 ** Supermodel is distributed in the hope that it will be useful, but WITHOUT
 ** ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 ** FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 ** more details.
 **
 ** You should have received a copy of the GNU General Public License along
 ** with Supermodel.  If not, see <http://www.gnu.org/licenses/>.
 **/
 
/*
 * Model3.h
 * 
 * Header file defining the CModel3 and CModel3Config classes.
 */

#ifndef INCLUDED_MODEL3_H
#define INCLUDED_MODEL3_H

#include "Model3/IEmulator.h"
#include "Model3/Crypto.h"

/*
 * FrameTimings
 *
 * Timings within a frame, for debugging purposes
 */
struct FrameTimings
{
  UINT32 ppcTicks;
  UINT32 syncSize;
  UINT32 syncTicks;
  UINT32 renderTicks;
  UINT32 sndTicks;
  UINT32 drvTicks;
  UINT32 frameTicks;
};

/*
 * CModel3Config:
 *
 * Settings used by CModel3.
 */
class CModel3Config
{
public:
  bool multiThreaded;    // Multi-threaded (enabled if true)
  bool gpuMultiThreaded; // Multi-threaded rendering (enabled if true)
  
  // PowerPC clock frequency in MHz (minimum: 1 MHz)
  inline void SetPowerPCFrequency(unsigned f)
  {
    if ((f<1) || (f>1000))
    {
      ErrorLog("PowerPC frequency must be between 1 and 1000 MHz; setting to 50 MHz.");
      f = 50;
    }   
    ppcFrequency = f*1000000;
  }
  inline unsigned GetPowerPCFrequency(void)
  {
    return ppcFrequency/1000000;
  }
  
  // Defaults
  CModel3Config(void)
  {
    multiThreaded = true;       // enable by default
    gpuMultiThreaded = true;    // enable by default
    ppcFrequency = 50*1000000;  // 50 MHz
  }
  
private:
  unsigned ppcFrequency;  // in Hz
};

/*
 * CModel3:
 *
 * A complete Model 3 system.
 *
 * Inherits IBus in order to pass the address space handlers to devices that
 * may need them (CPU, DMA, etc.)
 *
 * NOTE: Currently NOT re-entrant due to a non-OOP PowerPC core. Do NOT create
 * create more than one CModel3 object!
 */
class CModel3: public IEmulator, public IBus, public IPCIDevice
{
public:
  // IEmulator interface
  bool PauseThreads(void);
  bool ResumeThreads(void);
  void SaveState(CBlockFile *SaveState);
  void LoadState(CBlockFile *SaveState);
  void SaveNVRAM(CBlockFile *NVRAM);
  void LoadNVRAM(CBlockFile *NVRAM);
  void ClearNVRAM(void);
  void RunFrame(void);
  void RenderFrame(void);
  void Reset(void);
  const struct GameInfo * GetGameInfo(void);
  void AttachRenderers(CRender2D *Render2DPtr, IRender3D *Render3DPtr);
  void AttachInputs(CInputs *InputsPtr);  
  void AttachOutputs(COutputs *OutputsPtr);
  bool Init(void);

  // IPCIDevice interface
  UINT32 ReadPCIConfigSpace(unsigned device, unsigned reg, unsigned bits, unsigned width);
  void WritePCIConfigSpace(unsigned device, unsigned reg, unsigned bits, unsigned width, UINT32 data);

  // IBus interface
  UINT8 Read8(UINT32 addr);
  UINT16 Read16(UINT32 addr);
  UINT32 Read32(UINT32 addr);
  UINT64 Read64(UINT32 addr);
  void Write8(UINT32 addr, UINT8 data);
  void Write16(UINT32 addr, UINT16 data);
  void Write32(UINT32 addr, UINT32 data);
  void Write64(UINT32 addr, UINT64 data);
    
  /*
   * LoadROMSet(GameList, zipFile):
   *
   * Loads a complete ROM set from the specified ZIP archive.
   *
   * NOTE: Command line settings will not have been applied here yet.
   *
   * Parameters:
   *    GameList  List of all supported games and their ROMs.
   *    zipFile   ZIP file to load from.
   *
   * Returns:
   *    OKAY if successful, FAIL otherwise. Prints errors.
   */
  bool LoadROMSet(const struct GameInfo *GameList, const char *zipFile);

  /*
   * GetSoundBoard(void):
   * 
   * Returns a reference to the sound board.
   *
   * Returns:
   *    Pointer to CSoundBoard object.
   */
  CSoundBoard *GetSoundBoard(void);

  /*
   * GetDriveBoard(void):
   *
   * Returns a reference to the drive board.

   * Returns:
   *    Pointer to CDriveBoard object.
   */
  CDriveBoard *GetDriveBoard(void);

  /*
   * DumpTimings(void):
   *
   * Prints all timings for the most recent frame to the console, for debugging purposes.
   */
  void DumpTimings(void);

  /*
   * GetTimings(void):
   *
   * Returns timings for the most recent frame, for debugging purposes.
   */
  FrameTimings GetTimings(void);

  /*
   * CModel3(void):
   * ~CModel3(void):
   *
   * Constructor and destructor for Model 3 class. Constructor performs a 
   * bare-bones initialization of object; does not perform any memory 
   * allocation or any actions that can fail. The destructor will deallocate
   * memory and free resources used by the object (and its child objects).
   */
  CModel3(void);
  ~CModel3(void);

  /*
   * Private Property.
   * Tresspassers will be shot! ;)
   */
private:
  // Private member functions
  UINT8     ReadInputs(unsigned reg);
  void      WriteInputs(unsigned reg, UINT8 data);
  uint16_t  ReadSecurityRAM(uint32_t addr);
  UINT32    ReadSecurity(unsigned reg);
  void      WriteSecurity(unsigned reg, UINT32 data);
  void      SetCROMBank(unsigned idx);
  UINT8     ReadSystemRegister(unsigned reg);
  void      WriteSystemRegister(unsigned reg, UINT8 data);
  void      Patch(void);

  void RunMainBoardFrame(void);                       // Runs PPC main board for a frame
  void SyncGPUs(void);                                // Sync's up GPUs in preparation for rendering - must be called when PPC is not running
  bool RunSoundBoardFrame(void);                      // Runs sound board for a frame
  void RunDriveBoardFrame(void);                      // Runs drive board for a frame

  bool    StartThreads(void);                         // Starts all threads
  bool    StopThreads(void);                          // Stops all threads
  void    DeleteThreadObjects(void);                  // Deletes all threads and synchronization objects

  static int StartMainBoardThread(void *data);        // Callback to start PPC main board thread
  static int StartSoundBoardThread(void *data);       // Callback to start sound board thread (unsync'd)
  static int StartSoundBoardThreadSyncd(void *data);  // Callback to start sound board thread (sync'd)
  static int StartDriveBoardThread(void *data);       // Callback to start drive board thread

  static void AudioCallback(void *data);              // Audio buffer callback
  
  bool    WakeSoundBoardThread(void);                 // Used by audio callback to wake sound board thread (when not sync'd with render thread)
  int     RunMainBoardThread(void);                   // Runs PPC main board thread (sync'd in step with render thread)
  int     RunSoundBoardThread(void);                  // Runs sound board thread (not sync'd in step with render thread, ie running at full speed)
  int     RunSoundBoardThreadSyncd(void);             // Runs sound board thread (sync'd in step with render thread)
  int     RunDriveBoardThread(void);                  // Runs drive board thread (sync'd in step with render thread)
  
  // Game and hardware information
  const struct GameInfo *Game;
  
  // Game inputs and outputs
  CInputs   *Inputs;
  COutputs  *Outputs;
     
  // Input registers (game controls)
  UINT8   inputBank;
  UINT8   serialFIFO1, serialFIFO2;
  UINT8   gunReg;
  int     adcChannel;
  
  // MIDI port
  UINT8   midiCtrlPort; // controls MIDI (SCSP) IRQ behavior
  
  // Emulated core Model 3 memory regions
  UINT8   *memoryPool;  // single allocated region for all ROM and system RAM
  UINT8   *ram;         // 8 MB PowerPC RAM
  UINT8   *crom;        // 8+128 MB CROM (fixed CROM first, then 64MB of banked CROMs -- Daytona2 might need extra?)
  UINT8   *vrom;        // 64 MB VROM (video ROM, visible only to Real3D)
  UINT8   *soundROM;    // 512 KB sound ROM (68K program)
  UINT8   *sampleROM;   // 8 MB samples (68K)
  UINT8   *dsbROM;      // 128 KB DSB ROM (Z80 program)
  UINT8   *mpegROM;     // 8 MB DSB MPEG ROM
  UINT8   *backupRAM;   // 128 KB Backup RAM (battery backed)
  UINT8   *securityRAM; // 128 KB Security Board RAM
  UINT8   *driveROM;    // 32 KB drive board ROM (Z80 program) (optional)
  
  // Banked CROM
  UINT8     *cromBank;    // currently mapped in CROM bank
  unsigned  cromBankReg;  // the CROM bank register 
  
  // Security device
  bool      m_securityFirstRead = true;
  unsigned  securityPtr;  // pointer to current offset in security data
  
  // PowerPC
  PPC_FETCH_REGION  PPCFetchRegions[3];

  // Multiple threading
  bool        gpusReady;           // True if GPUs are ready to render
  bool        startedThreads;      // True if threads have been created and started
  bool        pauseThreads;        // True if threads should pause
  bool        stopThreads;         // True if threads should stop
  bool        syncSndBrdThread;    // True if sound board thread should be sync'd in step with render thread
  CThread     *ppcBrdThread;       // PPC main board thread
  CThread     *sndBrdThread;       // Sound board thread
  CThread     *drvBrdThread;       // Drive board thread
  bool        ppcBrdThreadRunning; // Flag to indicate PPC main board thread is currently processing
  bool        ppcBrdThreadDone;    // Flag to indicate PPC main board thread has finished processing
  bool        sndBrdThreadRunning; // Flag to indicate sound board thread is currently processing
  bool        sndBrdThreadDone;    // Flag to indicate sound board thread has finished processing
  bool        sndBrdWakeNotify;    // Flag to indicate that sound board thread has been woken by audio callback (when not sync'd with render thread)
  bool        drvBrdThreadRunning; // Flag to indicate drive board thread is currently processing
  bool        drvBrdThreadDone;    // Flag to indicate drive board thread has finished processing

  // Thread synchronization objects
  CSemaphore  *ppcBrdThreadSync;
  CSemaphore  *sndBrdThreadSync;
  CMutex      *sndBrdNotifyLock;
  CCondVar    *sndBrdNotifySync;
  CSemaphore  *drvBrdThreadSync;
  CMutex      *notifyLock;
  CCondVar    *notifySync;  
  
  // Frame timings
  FrameTimings timings;
  
  // Other devices
  CIRQ        IRQ;            // Model 3 IRQ controller
  CMPC10x     PCIBridge;      // MPC10x PCI/bridge/memory controller
  CPCIBus     PCIBus;         // Model 3's PCI bus
  C53C810     SCSI;           // NCR 53C810 SCSI controller
  CRTC72421   RTC;            // Epson RTC-72421 real-time clock
  C93C46      EEPROM;         // 93C46 EEPROM
  CTileGen    TileGen;        // Sega 2D tile generator
  CReal3D     GPU;            // Real3D graphics hardware
  CSoundBoard SoundBoard;     // Sound board
  CDSB        *DSB;           // Digital Sound Board (type determined dynamically at load time)
  CDriveBoard DriveBoard;     // Drive board
  CCrypto     m_cryptoDevice; // Encryption device
};


#endif  // INCLUDED_MODEL3_H