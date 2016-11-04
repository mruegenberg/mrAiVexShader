#define MODULE_API_EXPORTS
#include "vexvol_util.h"
#include <ai.h>
#include <string.h>
#include <math.h>
#include <CVEX/CVEX_Context.h>

bool VexVolVolumePluginInit(void** user_ptr)
{
   return true;
}

bool VexVolVolumePluginCleanup(void* user_ptr)
{
   return true;
}

// custom data structure for this type of volume DSO
struct VexVolVolumeData
{
    AtCritSec critSec;
    
    char codeFilename[255];
    CVEX_Context* ctxs[AI_MAX_THREADS]; // at most one context per thread
};

bool VexVolVolumePluginCreateVolume(
   void* user_ptr,
   const char* user_config,
   const AtNode* node,
   AtVolumeData* out_data)
{
   // allocate a custom data structure for this instance of the volume DSO
   VexVolVolumeData *sphere_data = (VexVolVolumeData*)AiMalloc(sizeof(VexVolVolumeData));

   // report the purpose of allocated memory to arnold
   AiAddMemUsage(sizeof(VexVolVolumeData), "sphere volume plugin data");

   for (int i=0; i<AI_MAX_THREADS; ++i)
   {
       sphere_data->ctxs[i] = 0;
   }

   float bbox_scale = 1.0f;
   
   std::string cfg_string(user_config);
   int space_pos = cfg_string.find(" ");
   if(space_pos < 0) {
       AiMsgWarning("Invalid cfg string?!");

       // directly use str as filename
       strncpy(sphere_data->codeFilename, user_config, std::min( strlen(user_config), (size_t)255) );
   }
   else {
       // parse
       std::string bbox_scale_s = cfg_string.substr(0, space_pos);
       std::string fileName_s = cfg_string.substr(space_pos + 1);

       AiMsgInfo(cfg_string.c_str());
       AiMsgInfo("pos: %d", space_pos);
       AiMsgInfo(bbox_scale_s.c_str());
       AiMsgInfo(fileName_s.c_str());
       
       bbox_scale = std::stof(bbox_scale_s);
       strncpy(sphere_data->codeFilename, fileName_s.c_str(), fileName_s.length() + 1); // + 1 for 
   }
   
   // AiNodeLookUpUserParameter(node, "file") ? AiNodeGetStr(node, "file") : "/home/marcel/Dropbox/mrAiVexVolumeProcedural/src/test2.vfl";
        
      // fill AtVolumeData structure
   out_data->private_info = sphere_data;

   // TODO: user should specify bounds themselves
   AtPoint mn; mn.x = -1 * bbox_scale; mn.y = -1 * bbox_scale; mn.z = -1 * bbox_scale;
   AtPoint mx; mx.x =  1 * bbox_scale; mx.y =  1 * bbox_scale; mx.z =  1 * bbox_scale;
   out_data->bbox.min = mn;
   out_data->bbox.max = mx;

   
   out_data->auto_step_size = AI_BIG;
   return true;
}

bool VexVolVolumePluginCleanupVolume(void* user_ptr, AtVolumeData* data, const AtNode* node)
{
   // free custom data structure of this instance of the volume DSO
   AiFree(data->private_info);

   // report the purpose of freed memory to arnold
   AiAddMemUsage(-sizeof(VexVolVolumeData), "sphere volume plugin data");
   return true;
}

bool VexVolVolumePluginSample(
   void* user_ptr,
   const AtVolumeData* voldata,
   const AtString channel,
   const AtShaderGlobals* sg,
   int interp,
   AtParamValue *value,
   AtByte *type)
{
    if (strcmp(channel, "density") != 0) // only handle density for now
        return false;

    VexVolVolumeData *data = (VexVolVolumeData*)(voldata->private_info);
    
    // context loading
    CVEX_Context *ctx = data->ctxs[sg->tid];
    if (!ctx)
    {
        ctx = new CVEX_Context();
        
        UT_String script(data->codeFilename);
        char *argv[4096];
        int argc = script.parse(argv, 4096);
        
        // default inputs translated from default Arnold names to default Hou names
        ctx->addInput("P",    CVEX_TYPE_VECTOR3, true); // P
        ctx->addInput("Time", CVEX_TYPE_FLOAT,   true); 
        
        if(! ctx->load(argc, argv)) {
            AiMsgWarning("loading the CVEX context failed. File: %s", data->codeFilename);
            return false;
        }
        else {
            AiMsgInfo("Loaded CVEX context.");
        }

        data->ctxs[sg->tid] = ctx;
    }

    // inputs
    UT_Vector3 vecBuffers[1]; // for simplicity, one vec buffer for all
    fpreal32 fltBuffers[1];
    CVEX_Value
        *P_val,
        *time_val;
    {
        P_val    = ctx->findInput("P",    CVEX_TYPE_VECTOR3);
        time_val = ctx->findInput("Time", CVEX_TYPE_FLOAT);
        
        if (P_val)
        {
            vecBuffers[0].assign(&(sg->P.x));
            P_val->setTypedData(vecBuffers + 0, 1);
        }
        else
            AiMsgWarning("could not find input P");
        
        if (time_val)
        {
            fltBuffers[1] = sg->time;
            time_val->setTypedData(fltBuffers + 0, 1);
        }
        else
            AiMsgWarning("could not find input Time");
    }

    // output
    CVEX_Value *out_val;
    float out[1];
    out_val = ctx->findOutput("density", CVEX_TYPE_FLOAT);
    if (out_val) {
        out_val->setTypedData(out, 1);
    }
    else
        AiMsgWarning("could not find output density");

    // run it
    ctx->run(1, false);

    // output
    *type = AI_TYPE_RGB;

    value->RGB = out[0];
    return true;
}

void VexVolVolumePluginRayExtents(
   void* user_ptr,
   const AtVolumeData* data,
   const AtVolumeIntersectionInfo* info,
   AtByte tid,
   float time,
   const AtPoint* origin,
   const AtVector* direction,
   float t0,
   float t1)
{
    AiVolumeAddIntersection(info, t0, t1);
}

AI_EXPORT_LIB bool VolumePluginLoader(AtVolumePluginVtable* vtable)
{
   vtable->Init           = VexVolVolumePluginInit;
   vtable->Cleanup        = VexVolVolumePluginCleanup;
   vtable->CreateVolume   = VexVolVolumePluginCreateVolume;
   vtable->CleanupVolume  = VexVolVolumePluginCleanupVolume;
   vtable->Sample         = VexVolVolumePluginSample;
   vtable->RayExtents     = VexVolVolumePluginRayExtents;
   strcpy(vtable->version, AI_VERSION);
   return true;
}
