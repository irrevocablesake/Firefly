# Firefly
FireFly, the insect that glows in the night - illuminating it's surroundings, just like the rays in this simulation elucidate a scene, stands as an inspiration for the name of this project. 

## To-do List:
This list stands as a rough guideline to ensure that we do not stray away from our goals:
- ⬜ OptiX Foundation
- ⬜ Brute Force PathTracer 
- ⬜ Material System
    - ⬜ BSDF
        - ⬜ BRDF
        - ⬜ BTDF
    - ⬜ BSSDF
- ⬜ Medium Traversal
- ⬜ Direct Lighting / NEE
- ⬜ MIS

## Author's Note:

It started with an idea to implement a GPU based PathTracer, which seemed like a straightforward option after [**Quasar**](https://github.com/irrevocablesake/Quasar) - CPU based PathTracer. So to give life to it, I began to translate the code from *Quasar* to *FireFly* achieved through the use of compute shaders.  Eventually though, I had an ephiphany that this approach would make me regurgitate what I learnt in *Quasar* and to no benefit, to make matters even worse - it would still be a software renderer with more parallization.

Due to the above insight, I decided that it's not worth spending time to translate and rather to use a existing **Hardware RT Pipeline**. That left me with 3 options:
- AMD HIPRT - No AMD GPU ( could have used it, but no affinity towards it )
- NVIDIA OptiX - Perfect choice
- Vulkan RT - Too much un-necessary management

Certainly, another factor with the conclusion of my choices reflects - that I want to focus more on Advanced Material Systems & Rendering Techniques. And for which a combination of Vulkan inter-op with OptiX ( CUDA ) gives me a good enough abstraction.
