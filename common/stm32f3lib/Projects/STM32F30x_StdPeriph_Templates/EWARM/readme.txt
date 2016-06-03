/**
  @page ewarm EWARM Project Template for STM32F3xx devices
  
  @verbatim
  ******************* (C) COPYRIGHT 2015 STMicroelectronics ********************
  * @file    readme.txt
  * @author  MCD Application Team
  * @version V1.2.2
  * @date    14-August-2015
  * @brief   This sub-directory contains all the user-modifiable files needed to
  *          create a new project linked with the STM32F30x Standard Peripherals 
  *          Library and working with IAR Embedded Workbench for ARM (EWARM)
  *          software toolchain.
  ******************************************************************************
  *
  * Licensed under MCD-ST Liberty SW License Agreement V2, (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        http://www.st.com/software_license_agreement_liberty_v2
  *
  * Unless required by applicable law or agreed to in writing, software 
  * distributed under the License is distributed on an "AS IS" BASIS, 
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
  ******************************************************************************
  @endverbatim
 
 @par Directory contents

 - project .ewd/.eww/.ewp: A pre-configured project file with the provided library 
                           structure that produces an executable image with IAR 
                           Embedded Workbench.
  
 - stm32f303xc_flash.icf : This file is the IAR Linker configuration file used to 
                         place program code (readonly) in internal FLASH and data
                         (readwrite, Stack and Heap)in internal SRAM. 
                         You can customize this file to your need.    
                         
 - stm32f303xc_ram.icf   : STM32F303xC devices  Linker configuration file 
                         used to place program code (readonly) and data (readwrite,
                         Stack and Heap)in internal SRAM.
                         You can customize this file to your need.
 
 - stm32f334x8_flash.icf : This file is the IAR Linker configuration file used to 
                         place program code (readonly) in internal FLASH and data
                         (readwrite, Stack and Heap)in internal SRAM. 
                         You can customize this file to your need.
                         
 - stm32f334x8_ram.icf   : STM32F334x8 devices  Linker configuration file 
                         used to place program code (readonly) and data (readwrite,
                         Stack and Heap)in internal SRAM.
                         You can customize this file to your need.

 - stm32f302x8_flash.icf : This file is the IAR Linker configuration file used to 
                         place program code (readonly) in internal FLASH and data
                         (readwrite, Stack and Heap)in internal SRAM. 
                         You can customize this file to your need.

 - stm32f302x8_ram.icf   : STM32F302x8 devices  Linker configuration file 
                         used to place program code (readonly) and data (readwrite,
                         Stack and Heap)in internal SRAM.
                         You can customize this file to your need.

 - stm32f303xe_flash.icf : This file is the IAR Linker configuration file used to 
                         place program code (readonly) in internal FLASH and data
                         (readwrite, Stack and Heap)in internal SRAM. 
                         You can customize this file to your need.     

 - stm32f303xe_ram.icf   : STM32F303xE devices  Linker configuration file 
                         used to place program code (readonly) and data (readwrite,
                         Stack and Heap)in internal SRAM.
                         You can customize this file to your need.
                             
 @par How to use it ?

 - Open the Project.eww workspace.
 - In the workspace toolbar select the project config:
     - STM32F303xC: to configure the project for STM32F303xC devices.
                    You can use STMicroelectronics STM32303C-EVAL board to run this project.
                  
     - STM32F334x8: to configure the project for STM32F334x8 devices.
                    You can use STMicroelectronics Discovery to run this project.
    
    - STM32F302x8: to configure the project for STM32F302x8 devices.
                   You can use STMicroelectronics Nucleo HW to run this project.
         
    - STM32F303xE: to configure the project for STM32F303xE devices.
                    You need to use custom HW board to run this project.
                    
     @note The needed define symbols for this config are already declared in the
           preprocessor section: STM32F3XXX, USE_STDPERIPH_DRIVER
           
 - Rebuild all files: Project->Rebuild all
 - Load project image: Project->Download and Debug
 - Run program: Debug->Go(F5) 

 * <h3><center>&copy; COPYRIGHT STMicroelectronics</center></h3>
 */