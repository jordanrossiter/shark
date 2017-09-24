/*
 * write_output.cpp

 *
 *  Created on: 18Sep.,2017
 *      Author: clagos
 */

#include <iomanip>
#include <iterator>
#include <memory>
#include <numeric>
#include <vector>
#include <iostream>
#include <fstream>

#include "components.h"
#include "cosmology.h"
#include "exceptions.h"
#include "logging.h"
#include "star_formation.h"
#include "write_output.h"
#include "utils.h"
#include "hdf5/writer.h"

using namespace std;

namespace shark {

WriteOutput::WriteOutput(ExecutionParameters exec_params, CosmologicalParameters cosmo_params, SimulationParameters sim_params, StarFormation starformation):
	exec_params(exec_params),
	sim_params(sim_params),
	cosmo_params(cosmo_params),
	starformation(starformation)
{
	//no-opt
}

void WriteOutput::write_galaxies(int snapshot, const std::vector<HaloPtr> &halos){

	string batch;

	if(exec_params.simulation_batches.size() == 1){
		batch = std::to_string(exec_params.simulation_batches[0]);
	}
	else{
		batch = "multiple_batches";
	}

	string fname = exec_params.output_directory + "/" + sim_params.sim_name + "/" + exec_params.name_model + "/" + std::to_string(snapshot) + "/" + batch + "/galaxies.hdf5";

	hdf5::Writer file(fname);

	file.write_dataset("runInfo/batches", exec_params.simulation_batches);
	file.write_attribute("runInfo/model_name", exec_params.name_model);
	file.write_attribute("runInfo/ode_solver_precision", exec_params.ode_solver_precision);
	file.write_attribute("runInfo/skip_missing_descendants", exec_params.skip_missing_descendants);
	file.write_attribute("runInfo/snapshot", snapshot);
	file.write_attribute("runInfo/redshift", sim_params.redshifts[snapshot]);

	// Calculate effective volume of the run
	float volume = sim_params.volume * exec_params.simulation_batches.size();

	file.write_attribute("runInfo/EffectiveVolume", volume);
	file.write_attribute("runInfo/particle_mass", sim_params.particle_mass);

	// Write cosmological parameters
	file.write_attribute("Cosmology/OmegaM", cosmo_params.OmegaM);
	file.write_attribute("Cosmology/OmegaB", cosmo_params.OmegaB);
	file.write_attribute("Cosmology/OmegaL", cosmo_params.OmegaL);
	file.write_attribute("Cosmology/n_s", cosmo_params.n_s);
	file.write_attribute("Cosmology/sigma8", cosmo_params.sigma8);
	file.write_attribute("Cosmology/h", cosmo_params.Hubble_h);

	// Create all galaxies properties I want to write
	vector<float> mstars_disk;
	vector<float> mstars_bulge;
	vector<float> mgas_disk;
	vector<float> mgas_bulge;
	vector<float> mstars_metals_disk;
	vector<float> mstars_metals_bulge;
	vector<float> mgas_metals_disk;
	vector<float> mgas_metals_bulge;
	vector<float> mmol_disk;
	vector<float> mmol_bulge;
	vector<float> matom_disk;
	vector<float> matom_bulge;

	vector<float> mBH;

	vector<float> sfr_disk;
	vector<float> sfr_burst;

	vector<float> rdisk;
	vector<float> rbulge;

	vector<float> mhot;
	vector<float> mhot_metals;

	vector<float> mreheated;
	vector<float> mreheated_metals;

	vector<float> mvir_hosthalo;
	vector<float> mvir_subhalo;
	vector<float> vmax_subhalo;
	vector<float> vvir_hosthalo;

	vector<float> cnfw_subhalo;

	vector<float> position_x;
	vector<float> position_y;
	vector<float> position_z;

	vector<float> velocity_x;
	vector<float> velocity_y;
	vector<float> velocity_z;

	vector<int> type;
	vector<long> id_halo;
	vector<long> id_subhalo;

	long i=1;
	long j=1;
	// Loop over all halos and subhalos to write galaxy properties
	for (auto &halo: halos){

		// assign properties of host halo
		auto mhalo = halo->Mvir;
		auto vhalo = halo->Vvir;

		for (auto &subhalo: halo->all_subhalos()){

			// assign properties of host subhalo
			auto msubhalo = subhalo->Mvir;
			auto vsubhalo = subhalo->Vcirc;
			auto cnfw = subhalo->concentration;
			auto subhalo_position = subhalo->position;
			auto subhalo_velocity = subhalo->velocity;

			// Assign baryon properties of subhalo
			auto hot_subhalo = subhalo->hot_halo_gas;
			auto cold_subhalo = subhalo->cold_halo_gas;
			auto reheated_subhalo = subhalo->ejected_galaxy_gas;

			for (auto &galaxy: subhalo->galaxies){

				//Calculate molecular gass mass of disk and bulge:
				double m_mol = 0;
				double m_atom = 0;
				double m_mol_b = 0;
				double m_atom_b = 0;
				if(galaxy->disk_gas.mass > 0){
					m_mol = starformation.molecular_hydrogen(galaxy->disk_gas.mass,galaxy->disk_stars.mass,galaxy->disk_gas.rscale, galaxy->disk_stars.rscale, sim_params.redshifts[snapshot]);
					m_atom = galaxy->disk_gas.mass - m_mol;
				}
				if(galaxy->bulge_gas.mass > 0){
					m_mol_b = starformation.molecular_hydrogen(galaxy->bulge_gas.mass,galaxy->bulge_stars.mass,galaxy->bulge_gas.rscale, galaxy->bulge_stars.rscale, sim_params.redshifts[snapshot]);
					m_atom_b = galaxy->bulge_gas.mass - m_mol_b;
				}

				mmol_disk.push_back(m_mol);
				mmol_bulge.push_back(m_mol_b);
				matom_disk.push_back(m_atom);
				matom_bulge.push_back(m_atom_b);

				mstars_disk.push_back(galaxy->disk_stars.mass);
				mstars_bulge.push_back(galaxy->bulge_stars.mass);
				mgas_disk.push_back(galaxy->disk_gas.mass);
				mgas_bulge.push_back(galaxy->bulge_gas.mass);
				mstars_metals_disk.push_back(galaxy->disk_stars.mass_metals);
				mstars_metals_bulge.push_back(galaxy->bulge_stars.mass_metals);
				mgas_metals_disk.push_back(galaxy->disk_gas.mass_metals);
				mgas_metals_bulge.push_back(galaxy->bulge_gas.mass_metals);
				sfr_disk.push_back(galaxy->sfr_disk);
				sfr_burst.push_back(galaxy->sfr_bulge);
				mBH.push_back(galaxy->smbh.mass);

				rdisk.push_back(galaxy->disk_stars.rscale);
				rbulge.push_back(galaxy->bulge_stars.rscale);

				double mhot_gal = 0;
				double mzhot_gal = 0;
				double mreheat = 0;
				double mzreheat =0;
				int t = 2;
				if(galaxy->galaxy_type == Galaxy::CENTRAL){
					t = 0;
					mhot_gal = hot_subhalo.mass + cold_subhalo.mass;
					mzhot_gal = hot_subhalo.mass_metals + cold_subhalo.mass_metals;
					mreheat = reheated_subhalo.mass;
					mzreheat = reheated_subhalo.mass_metals;
				}
				else if(galaxy->galaxy_type == Galaxy::TYPE1){
					t=1;
				}

				mhot.push_back(mhot_gal);
				mhot_metals.push_back(mzhot_gal);
				mreheated.push_back(mreheat);
				mreheated_metals.push_back(mzreheat);

				mvir_hosthalo.push_back(mhalo);

				double mvir_gal = 0 ;
				double vmax_sub = 0;
				double c_sub = 0;
				xyz<float> pos;
				xyz<float> vel;

				if(galaxy->galaxy_type == Galaxy::CENTRAL || galaxy->galaxy_type == Galaxy::TYPE1){
					mvir_gal = msubhalo;
					vmax_sub = vsubhalo;
					c_sub = cnfw;
					pos = subhalo_position;
					vel = subhalo_velocity;
				}
				else{
					// In case of type 2 galaxies assign negative positions and velocities.
					pos.x = -1;
					pos.y = -1;
					pos.z = -1;
					vel.x = -1;
					vel.y = -1;
					vel.z = -1;
				}

				mvir_subhalo.push_back(mvir_gal);
				vmax_subhalo.push_back(vmax_sub);
				vvir_hosthalo.push_back(vhalo);
				cnfw_subhalo.push_back(c_sub);

				position_x.push_back(pos.x);
				position_y.push_back(pos.y);
				position_z.push_back(pos.z);

				velocity_x.push_back(vel.x);
				velocity_y.push_back(vel.y);
				velocity_z.push_back(vel.z);

				type.push_back(t);

				id_halo.push_back(j);
				id_subhalo.push_back(i);
			}
			i++;
		}
		j++;
	}

	file.write_dataset("Galaxies/mstars_disk", mstars_disk);
	file.write_dataset("Galaxies/mstars_bulge", mstars_bulge);
	file.write_dataset("Galaxies/mgas_disk", mgas_disk);
	file.write_dataset("Galaxies/mgas_bulge", mgas_bulge);
	file.write_dataset("Galaxies/mstars_metals_disk",mstars_metals_disk);
	file.write_dataset("Galaxies/mstars_metals_bulge", mstars_metals_bulge);
	file.write_dataset("Galaxies/mgas_metals_disk", mgas_metals_disk);
	file.write_dataset("Galaxies/mgas_metals_bulge", mgas_metals_bulge);
	file.write_dataset("Galaxies/mmol_disk",mmol_disk);
	file.write_dataset("Galaxies/mmol_bulge",mmol_bulge);
	file.write_dataset("Galaxies/matom_disk",matom_disk);
	file.write_dataset("Galaxies/matom_bulge",matom_bulge);

	file.write_dataset("Galaxies/mBH", mBH);

	file.write_dataset("Galaxies/rdisk", rdisk);
	file.write_dataset("Galaxies/rbulge", rbulge);

	file.write_dataset("Galaxies/mhot", mhot);
	file.write_dataset("Galaxies/mhot_metals", mhot_metals);
	file.write_dataset("Galaxies/mreheated", mreheated);
	file.write_dataset("Galaxies/mreheated_metals", mreheated_metals);

	file.write_dataset("Galaxies/mvir_hosthalo", mvir_hosthalo);
	file.write_dataset("Galaxies/mvir_subhalo", mvir_subhalo);
	file.write_dataset("Galaxies/vmax_subhalo", vmax_subhalo);
	file.write_dataset("Galaxies/vvir_hosthalo", vvir_hosthalo);
	file.write_dataset("Galaxies/cnfw_subhalo", cnfw_subhalo);

	file.write_dataset("Galaxies/position_x", position_x);
	file.write_dataset("Galaxies/position_y", position_y);
	file.write_dataset("Galaxies/position_z", position_z);

	file.write_dataset("Galaxies/velocity_x", velocity_x);
	file.write_dataset("Galaxies/velocity_y", velocity_y);
	file.write_dataset("Galaxies/velocity_z", velocity_z);

	file.write_dataset("Galaxies/type", type);

	file.write_dataset("Galaxies/id_subhalo", id_subhalo);
	file.write_dataset("Galaxies/id_halo", id_halo);

	string fname_ascii = exec_params.output_directory + "/" + sim_params.sim_name + "/" + exec_params.name_model + "/" + std::to_string(snapshot) + "/" + batch + "/galaxies.dat";

	fstream file_ascii;
	file_ascii.open(fname_ascii, ios_base::out);

	for (int i = 0; i <= mstars_disk.size()-1; i++){
		file_ascii << mstars_disk[i] << " " << mstars_bulge[i] << " " << mgas_disk[i] << " " << mgas_metals_disk[i] <<" " << mgas_bulge[i] << " " << mBH[i] << " " << mhot[i] << " " << mreheated[i] << " " << mstars_disk[i]+mstars_bulge[i] << " " << type[i] << " " << id_halo[i] << " " << id_subhalo[i] << std::endl;
	}

	file_ascii.close();



}

}// namespace shark

