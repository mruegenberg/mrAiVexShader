/*
 * solid texture shader
 */

#include <ai.h>
#include <string.h>
#include <cstdio>
#include <CVEX/CVEX_Context.h>
#include <UT/UT_Vector3.h>
#include <MOT/MOT_Director.h>

enum SolidtexParams {    
    p_vexFile,
    p_vexCode,
    p_vexSource,
    p_paramNames,
    p_paramValues,
};

#define TRUE 1
#define FALSE 0

AI_SHADER_NODE_EXPORT_METHODS(VexShdMethods);

node_parameters
{        
    AiParameterStr("vexFile", "/opt/hfs15.5.607/toolkit/samples/CVEX/simple.vfl");
    AiParameterStr("vexCode", "");
    AiParameterStr("vexSource", 0);
    AiParameterArray("ExtraParamNames", AiArrayAllocate(0, 0, AI_TYPE_STRING));
    AiParameterArray("ExtraParamValues", AiArrayAllocate(0, 0, AI_TYPE_FLOAT));
}

struct ShaderData {
    AtCritSec critSec;
    
    char codeFilename[L_tmpnam + 4];
    CVEX_Context* ctxs[AI_MAX_THREADS]; // at most one context per thread
};

node_initialize
{
    // we use placement new syntax to have both the constructor (partcularly for CVEX_Context)
    // called and memory allocated by Arnold's internal malloc
    // by Arnold
    ShaderData* data = new (AiMalloc(sizeof(ShaderData))) ShaderData();
    for (int i=0; i<AI_MAX_THREADS; ++i)
    {
        data->ctxs[i] = 0;
    }
    AiCritSecInit(&(data->critSec));
    AiNodeSetLocalData(node, data);
}

node_update
{
    ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);
    for (int i=0; i<AI_MAX_THREADS; ++i) // invalidate all ctxs
    {
        if (data->ctxs[i])
        {
            delete data->ctxs[i];
            data->ctxs[i] = 0;
        }
    }
}

node_finish
{
    ShaderData *data = (ShaderData*)AiNodeGetLocalData(node);

    if (data)
    {
        AiCritSecClose(&(data->critSec));
        data->~ShaderData(); // call constructor without dealloc
        AiFree((void*) data);
        AiNodeSetLocalData(node, NULL);
    }
}

node_loader
{
    if (i > 0) return FALSE;
   
    node->methods      = VexShdMethods;
    node->output_type  = AI_TYPE_RGB;
    node->name         = "vexrgb";
    node->node_type    = AI_NODE_SHADER;
    strcpy(node->version, AI_VERSION);
    return TRUE;
};


shader_evaluate
{
    sg->out.RGB = AI_RGB_RED; // dummy

    ShaderData* data = (ShaderData*) AiNodeGetLocalData(node);

    // context
    CVEX_Context *ctx = data->ctxs[sg->tid];
    if (!ctx)
    {
        ctx = new CVEX_Context();
        data->ctxs[sg->tid] = ctx;

        const char *vexSourceType = AiShaderEvalParamStr(p_vexSource);
        const char *fileName;
        if(strcmp(vexSourceType, "code") == 0) { // existing code. copy to a file.
            AiCritSecEnter(&(data->critSec)); // tmpnam is not threadsafe
            std::tmpnam(data->codeFilename); // TODO: add pragma to silence the unsafe warning?
            strcat(data->codeFilename, ".vfl");
            fileName = data->codeFilename;
            AiMsgWarning("fn: %s", fileName);
            AiCritSecLeave(&(data->critSec));

            const char *vexCode = AiShaderEvalParamStr(p_vexCode);
            std::ofstream out(fileName);
            out << vexCode;
            out.close();
        }
        else {
            fileName = AiShaderEvalParamStr(p_vexFile);
        }
        
        UT_String script(fileName);
        char *argv[4096];
        int argc = script.parse(argv, 4096);

        // default inputs translated from default Arnold names to default Hou names
        ctx->addInput("P",    CVEX_TYPE_VECTOR3, true); // P
        ctx->addInput("Eye",  CVEX_TYPE_VECTOR3, true); // Ro
        ctx->addInput("I",    CVEX_TYPE_VECTOR3, true); // Rd
        ctx->addInput("dPds", CVEX_TYPE_VECTOR3, true); // dPdu
        ctx->addInput("dPdt", CVEX_TYPE_VECTOR3, true); // dPdv
        ctx->addInput("N",    CVEX_TYPE_VECTOR3, true); // N
        ctx->addInput("Ng",   CVEX_TYPE_VECTOR3, true); // Ng
        ctx->addInput("s",    CVEX_TYPE_FLOAT,   true); // u
        ctx->addInput("t",    CVEX_TYPE_FLOAT,   true); // v

        // user params
        AtArray *paramNames = AiShaderEvalParamArray(p_paramNames);
        for(int i=0; i<paramNames->nelements; ++i) {
            const char *paramName = AiArrayGetStr(paramNames, i);
            ctx->addInput(paramName, CVEX_TYPE_FLOAT, false); // false = not varying = same for all pts
        }

        if(! ctx->load(argc, argv)) {
            AiMsgWarning("loading the CVEX context failed");
            return;
        }
    }

    UT_Vector3 vecBuffers[7]; // for simplicity, one vec buffer for all
    fpreal32 fltBuffers[2];
    // TODO: maybe don't reallocate these in each call? CVex_Value probably just keeps a pointer,
    //       so we'd have to do the setTypedData call only once, saving both on [stack] alloc and unneeded
    //       assigns.

    // SG params
    CVEX_Value
        *P_val,
        *Eye_val, *I_val,
        *dPds_val, *dPdt_val,
        *N_val, *Ng_val,
        *s_val, *t_val;
    {
        P_val    = ctx->findInput("P",    CVEX_TYPE_VECTOR3);
        Eye_val  = ctx->findInput("Eye",  CVEX_TYPE_VECTOR3);
        I_val    = ctx->findInput("I",    CVEX_TYPE_VECTOR3);
        dPds_val = ctx->findInput("dPds", CVEX_TYPE_VECTOR3);
        dPdt_val = ctx->findInput("dPdt", CVEX_TYPE_VECTOR3);
        N_val    = ctx->findInput("N",    CVEX_TYPE_VECTOR3);
        Ng_val   = ctx->findInput("Ng",   CVEX_TYPE_VECTOR3);
        s_val    = ctx->findInput("s",    CVEX_TYPE_FLOAT);
        t_val    = ctx->findInput("t",    CVEX_TYPE_FLOAT);
        
        if (P_val)
        {
            vecBuffers[0].assign(&(sg->P.x));
            P_val->setTypedData(vecBuffers + 0, 1);
        }
        if (Eye_val)
        {
            vecBuffers[1].assign(&(sg->Ro.x));
            Eye_val->setTypedData(vecBuffers + 1, 1);
        }
        if (I_val)
        {
            vecBuffers[2].assign(&(sg->Rd.x));
            I_val->setTypedData(vecBuffers + 2, 1);
        }
        if (dPds_val)
        {
            vecBuffers[3].assign(&(sg->dPdu.x));
            dPds_val->setTypedData(vecBuffers + 3, 1);
        }
        if (dPdt_val)
        {
            vecBuffers[4].assign(&(sg->dPdv.x));
            dPdt_val->setTypedData(vecBuffers + 4, 1);
        }
        if (N_val)
        {
            vecBuffers[5].assign(&(sg->N.x));
            N_val->setTypedData(vecBuffers + 5, 1);
        }
        if (Ng_val)
        {
            vecBuffers[6].assign(&(sg->Ng.x));
            Ng_val->setTypedData(vecBuffers + 6, 1);
        }

        if (s_val)
        {
            fltBuffers[0] = sg->u;
            s_val->setTypedData(fltBuffers + 0, 1);
        }
        if (t_val)
        {
            fltBuffers[1] = sg->v;
            t_val->setTypedData(fltBuffers + 1, 1);
        }
    }
        

    // user params
    AtArray *paramNames = AiShaderEvalParamArray(p_paramNames);
    AtArray *paramVals  = AiShaderEvalParamArray(p_paramValues);
    std::vector<fpreal32> userFltBuffer(paramNames->nelements, 0.0f);                                        
    {
        int c = std::min(paramNames->nelements, paramVals->nelements);
        for(int i=0; i<c; ++i) {
            userFltBuffer[i] = AiArrayGetFlt(paramVals, i);
        }   
        for(int i=0; i<paramNames->nelements; ++i) {
            const char *paramName = AiArrayGetStr(paramNames, i);
            CVEX_Value *param = ctx->findInput(paramName, CVEX_TYPE_FLOAT);
            if(param) {
                param->setTypedData(&(userFltBuffer[i]), 1);
            }
        }
    }       

    CVEX_Value *out_val;
    UT_Vector3 out[1];
    out_val = ctx->findOutput("out", CVEX_TYPE_VECTOR3);
    if (out_val) {
        out_val->setTypedData(out, 1);
    }

    ctx->run(1, false);

    sg->out.RGB = AiColorCreate((float)(out[0].x()),(float)(out[0].y()),(float)(out[0].z()));
}

