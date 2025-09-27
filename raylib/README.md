
## What has been modified:

```
* Material unloading.
    file: "raylib/src/rmodels.c"
    function: "UnloadMaterial"

    Check for negative shader id values was added before unloading shaders.
    'bool should_unload_shader' was added because 
    sometimes it is unwanted behaviour to load the shader at the sametime.
    Also macros RL_UNLOAD_MAT_SHADER and RL_DONT_UNLOAD_MAT_SHADER was added too for readability.


* Shader unloading.
    file: "raylib/src/rcore.c"
    function: "UnloadShader"

    Function takes shader as pointer now.
    shader->id is checked for negative values before unloading.
    shader->id is set to -1 when unloaded.
    shader->locs is set to NULL when unloadedo.
    
```


