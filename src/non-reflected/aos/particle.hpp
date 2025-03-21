#pragma once

#include <utils/constants.hpp>


#include "math/math.hpp"

#include <utils/primitive_types.hpp>

namespace sim {

struct ParticlesData {
  explicit ParticlesData(math::scalar const ppm) :
    particles_per_meter(ppm), smoothing(mul_rad / ppm), smoothing_pow_2(pow(smoothing, 2)),
    smoothing_pow_6(pow(smoothing_pow_2, 3)), smoothing_pow_9(pow(smoothing, 9)), mass(density / pow(ppm, 3)),
    f45_pi_smooth_6(45 / (std::numbers::pi * smoothing_pow_6)), mass_pressure_05(mass * pressure * 0.5),
    mass_goo(goo * mass), transform_density_constant((315.0 / (pi_times_64 * smoothing_pow_9)) * mass) { }


  math::scalar particles_per_meter;
  math::scalar smoothing;
  math::scalar smoothing_pow_2;
  math::scalar smoothing_pow_6;
  math::scalar smoothing_pow_9;
  math::scalar mass;
  math::scalar f45_pi_smooth_6;
  math::scalar mass_pressure_05;
  math::scalar mass_goo;
  math::scalar transform_density_constant;
};

struct Particle {
  Particle(u64 const id, math::vec3 const pos, math::vec3 const hv, math::vec3 const vel) :
    id(id), position(pos), hv(hv), velocity(vel), acceleration(gravity) { }

  Particle(Particle const& other) :
    id(other.id), position(other.position), hv(other.hv), velocity(other.velocity), acceleration(gravity), density(0) {
  }

  void transformDensity(ParticlesData const& particles_params) {
    density = (density + particles_params.smoothing_pow_6) * particles_params.transform_density_constant;
  }

  u64 id;
  math::vec3 position;
  math::vec3 hv;
  math::vec3 velocity;
  math::vec3 acceleration {gravity};
  math::scalar density {0};
};

inline math::scalar densityIncrement(ParticlesData const& particles_params, math::scalar const squared_distance) {
  return std::pow(particles_params.smoothing_pow_2 - squared_distance, 3);
}

/**
 * Esta función calcula el incremento de aceleración entre dos partículas, `particle_i` y `particle_j`,
 * en función de los parámetros especificados en `params`. El incremento de aceleración se calcula teniendo en cuenta
 * la distancia al cuadrado entre las partículas.
 *
 * @param params Los parámetros de las partículas y la simulación.
 * @param particle_i La primera partícula para la que se calculará el incremento de aceleración.
 * @param particle_j La segunda partícula para la que se calculará el incremento de aceleración.
 * @param squared_distance La distancia al cuadrado entre las partículas.
 * @return Un vector 3D que representa el incremento de aceleración entre las partículas.
 */
inline auto accelerationIncrement(
    ParticlesData const& params, Particle const& particle_i, Particle const& particle_j,
    math::scalar const squared_distance
) {

  math::scalar const distance = squared_distance > min_distance ? std::sqrt(squared_distance) : min_distance_sqrt;
  math::vec3 const left       = (particle_i.position - particle_j.position) * params.mass_pressure_05 *
                          (std::pow(params.smoothing - distance, 2) / distance) *
                          (particle_i.density + particle_j.density - density_times_2);

  math::vec3 const right         = (particle_j.velocity - particle_i.velocity) * params.mass_goo;
  math::scalar const denominator = particle_i.density * particle_j.density;
  return ((left + right) * params.f45_pi_smooth_6 / denominator);
}

/**
 * Calcula un incremento en las densidades de dos partículas si su distancia
 * al cuadrado es menor que el suavizado al cuadrado proporcionado en los parámetros de partículas.
 * @param particles_params Parámetros de partículas que incluyen información sobre el suavizado.
 * @param particle_i Primera partícula.
 * @param particle_j Segunda partícula.
 */
inline void incrementDensities(ParticlesData const& particles_params, Particle& particle_i, Particle& particle_j) {
  if (math::scalar const squared_distance = squaredDistance(particle_i.position, particle_j.position);
      squared_distance < particles_params.smoothing_pow_2) {
    // Incremento de densidades basado en la distancia
    math::scalar const density_increment = densityIncrement(particles_params, squared_distance);
    particle_i.density += density_increment;
    particle_j.density += density_increment;
  }
}

/**
 * Esta función calcula un incremento en las aceleraciones de dos partículas si su distancia
 * al cuadrado es menor que el suavizado al cuadrado proporcionado en los parámetros de partículas.
 * @param particles_params Parámetros de partículas que incluyen información sobre el suavizado.
 * @param particle_i Primera partícula.
 * @param particle_j Segunda partícula.
 */
inline void incrementAccelerations(ParticlesData const& particles_params, Particle& particle_i, Particle& particle_j) {
  if (math::scalar const squared_distance = squaredDistance(particle_i.position, particle_j.position);
      squared_distance < particles_params.smoothing_pow_2) {
    math::vec3 const incr = accelerationIncrement(particles_params, particle_i, particle_j, squared_distance);
    particle_i.acceleration += incr;
    particle_j.acceleration -= incr;
  }
}

} // namespace sim
