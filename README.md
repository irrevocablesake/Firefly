# Firefly
Compute Shader Based Ray Tracing Engine

FireFly, the insect that glows in the night - illuminating it's surroundings, just like the rays in this simulation elucidate a scene, stands as an inspiration for the name of this project. 

## To-do List:
This list stands as a rough guideline to ensure that we do not stray away from our goals:
- <input type="checkbox" disabled> OptiX Foundation
- <input type="checkbox" disabled> Brute Force PathTracer 
- <input type="checkbox" disabled> Material System
    - <input type="checkbox" disabled> BSDF
        - <input type="checkbox" disabled> BRDF
        - <input type="checkbox" disabled> BTDF
    - <input type="checkbox" disabled> BSSDF
- <input type="checkbox" disabled> Medium Traversal
- <input type="checkbox" disabled> Direct Lighting / NEE
- <input type="checkbox" disabled> MIS

## Author's Note:

It started with an idea to implement a GPU based PathTracer, which seemed like a straightforward option after [**Quasar**](https://github.com/irrevocablesake/Quasar) - CPU based PathTracer. So to give life to it, I began to translate the code from *Quasar* to *FireFly* achieved through the use of compute shaders.  Eventually though, I had an ephiphany that this approach would make me regurgitate what I learnt in *Quasar* and to no benefit, to make matters even worse - it would still be a software renderer with more parallization.

Due to the above insight, I decided that it's not worth spending time to translate and rather to use a existing **Hardware RT Pipeline**. That left me with 3 options:
- AMD HIPRT - No AMD GPU ( could have used it, but no affinity towards it )
- NVIDIA OptiX - Perfect choice
- Vulkan RT - Too much un-necessary management

Certainly, another factor with the conclusion of my choices reflects - that I want to focus more on Advanced Material Systems & Rendering Techniques. And for which a combination of Vulkan inter-op with OptiX ( CUDA ) gives me a good enough abstraction.