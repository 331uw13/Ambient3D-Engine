#ifndef AMBIENT3D_SERVER_NOISE_GENERATOR_HPP
#define AMBIENT3D_SERVER_NOISE_GENERATOR_HPP


namespace AM {


    struct NoiseGenCFG {

        // Alternation aka "alt" is change to the amplitude of noise function.
        // For example when computing detail:
        // alt is 1.0 to -1.0 (usually very low frequency) perlin noise 
        // then detail result is: detail * detail_alt.
        // "Detail" is high frequency low amplitude noise.
        // This gives effect that amount of detail changes.

        float base_frq; // Frequency.
        float base_amp; // Amplitude
        float base_alt; // Alternation.
        float base_detail_frq;
        float base_detail_amp;
        float base_detail_alt;

        float fields_frq;
        float fields_amp;

        float mountain_frq;
        float mountain_amp;
        float mountain_alt;
        int   mountain_iterations;
        float mountain_iteration_frq_add;  // Values to add each iteration.
        float mountain_iteration_amp_add;  //
        float mountain_iteration_alt_add;  //

        // TODO: Rivers?
    };

    class NoiseGen {
        public:

            NoiseGen() {}
            NoiseGen(const NoiseGenCFG& _cfg) : m_cfg(_cfg)  {}

            // World X and Z
            const float get_noise(float x, float z) const;


        private:

            NoiseGenCFG m_cfg;
    };

};



#endif
