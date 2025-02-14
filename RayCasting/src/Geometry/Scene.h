#ifndef _Geometry_Scene_H
#define _Geometry_Scene_H

#include <windows.h>
#include <Geometry/Geometry.h>
#include <Geometry/PointLight.h>
#include <Visualizer/Visualizer.h>
#include <Geometry/Camera.h>
#include <Geometry/BoundingBox.h>
#include <Math/RandomDirection.h>
#include <windows.h>
#include <System/aligned_allocator.h>
#include <Math/Constant.h>
#include <queue>
#include <functional>
#include <random>
#include <Geometry/LightSampler.h>
#include <Geometry/BVH.h>
#include <Geometry/LightSource.h>
#include <ctime>
#include <Math/RandomDirection.h>

namespace Geometry
{
	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// \class	Scene
	///
	/// \brief	An instance of a geometric scene that can be rendered using ray casting. A set of methods
	/// 		allowing to add geometry, lights and a camera are provided. Scene rendering is achieved by
	/// 		calling the Scene::compute method.
	///
	/// \author	F. Lamarche, Universit� de Rennes 1
	/// \date	03/12/2013
	////////////////////////////////////////////////////////////////////////////////////////////////////
	class Scene
	{
	protected:
		/// \brief	The visualizer (rendering target).
		Visualizer::Visualizer * m_visu ;
		/// \brief	The scene geometry (basic representation without any optimization).
		::std::deque<::std::pair<BoundingBox, Geometry> > m_geometries ;
		//Geometry m_geometry ;
		/// \brief	The lights.
		std::vector<PointLight> m_lights ;
		/// \brief	The camera.
		Camera m_camera ;
		/// \brief The scene bounding box
		BoundingBox m_sceneBoundingBox;
		/// \brief number of diffuse samples for global illumination
		size_t m_diffuseSamples ;
		/// \brief Number of specular samples 
		size_t m_specularSamples ;
		/// \brief Number of light samples (per light)
		size_t m_lightSamples;
		/// brief Rendering pass number
		int m_pass;
		/// <summary>
		/// The light sampler associated with the scene
		/// </summary>

		//LightSampler m_lightSampler;
		//Les sources surfaciques de lumiere de la scene
		::std::vector<LightSource*> m_lightSampler;
		//La structure d'optimisation qui va permettre d'optimiser le calcul d'intersections
		BVH *m_bvh;
		//******GI
		//Activer ou desactiver l'illumination globale
		bool m_GI_surface = true;
		//Activer ou desactiver l'echantillonnage a graine unique
		bool m_GI_graineUnique = false; // NE PAS TOUCHE MAMA
		//pathtracing
		bool m_GI_indirect = true;


	public:

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// \fn	Scene::Scene(Visualizer::Visualizer * visu)
		///
		/// \brief	Constructor.
		///
		/// \author	F. Lamarche, Universit� de Rennes 1
		/// \date	03/12/2013
		///
		/// \param [in,out]	visu	If non-null, the visu.
		////////////////////////////////////////////////////////////////////////////////////////////////////
		Scene(Visualizer::Visualizer * visu)
			: m_visu(visu), m_diffuseSamples(30), m_specularSamples(30), m_lightSamples(0), m_bvh(nullptr)
		{}

		/// <summary>
		/// Prints stats about the geometry associated with the scene
		/// </summary>
		void printStats()
		{
			size_t nbTriangles = 0;
			for (auto it = m_geometries.begin(), end = m_geometries.end(); it != end; ++it)
			{
				nbTriangles += it->second.getTriangles().size();
			}
			::std::cout << "Scene: " << nbTriangles << " triangles" << ::std::endl;
		}

		/// <summary>
		/// Computes the scene bounding box.
		/// </summary>
		/// <returns></returns>
		const BoundingBox & getBoundingBox()
		{
			m_geometries;
			return m_sceneBoundingBox;
		}

		/// <summary>
		/// Sets the number of diffuse samples
		/// </summary>
		/// <param name="number"> The number of diffuse samples</param>
		void setDiffuseSamples(size_t number)
		{
			m_diffuseSamples = number;
		}

		/// <summary>
		/// Sets the number of specular samples
		/// </summary>
		/// <param name="number"></param>
		void setSpecularSamples(size_t number)
		{
			m_specularSamples = number;
		}

		/// <summary>
		/// Sets the number of light samples if the scene contains surface area lights
		/// </summary>
		/// <param name="number">The number of samples</param>
		void setLightSamples(size_t number)
		{
			m_lightSamples = number;
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// \fn	void Scene::add(const Geometry & geometry)
		///
		/// \brief	Adds a geometry to the scene.
		///
		/// \author	F. Lamarche, Universit� de Rennes 1
		/// \date	03/12/2013
		///
		/// \param	geometry The geometry to add.
		////////////////////////////////////////////////////////////////////////////////////////////////////
		void add(const Geometry & geometry)
		{
			if (geometry.getVertices().size() == 0) { return; }
			BoundingBox box(geometry) ;
			m_geometries.push_back(::std::make_pair(box, geometry)) ;
			m_geometries.back().second.computeVertexNormals(Math::piDiv4/2);
			if (m_geometries.size() == 1)
			{
				m_sceneBoundingBox = box;
			}
			else
			{
				m_sceneBoundingBox.update(box);
			}
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// \fn	void Scene::add(Light & light)
		///
		/// \brief	Adds a light source in the scene.
		///
		/// \author	F. Lamarche, Universit� de Rennes 1
		/// \date	04/12/2013
		///
		/// \param [in,out]	light	If non-null, the light to add.
		////////////////////////////////////////////////////////////////////////////////////////////////////
		void add(const PointLight & light) 
		{
			m_lights.push_back(light);
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// \fn	void Scene::add(LightSource * light)
		///
		/// \brief	Adds a light source in the scene.
		///
		/// \author	F. Lamarche, Universit� de Rennes 1
		/// \date	04/12/2013
		///
		/// \param [in,out]	light	If non-null, the light to add.
		////////////////////////////////////////////////////////////////////////////////////////////////////
		void add(LightSource *light)
		{
			m_lightSampler.push_back(light);
			add(*light);			
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// \fn	void Scene::setCamera(Camera const & cam)
		///
		/// \brief	Sets the camera.
		///
		/// \author	F. Lamarche, Universit� de Rennes 1
		/// \date	04/12/2013
		///
		/// \param	cam	The camera.
		////////////////////////////////////////////////////////////////////////////////////////////////////
		void setCamera(Camera const & cam)
		{
			m_camera = cam ;
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// \fn	RGBColor Scene::sendRay(Ray const & ray, double limit, int depth, int maxDepth)
		///
		/// \brief	Sends a ray in the scene and returns the computed color
		///
		/// \author	F. Lamarche, Universit� de Rennes 1
		/// \date	04/12/2013
		///
		/// \param	ray			The ray.
		/// \param	depth   	The current depth.
		/// \param	maxDepth	The maximum depth.
		///
		/// \return	The computed color.
		////////////////////////////////////////////////////////////////////////////////////////////////////
		RGBColor sendRay(Ray const & ray, int depth, int maxDepth, int diffuseSamples, int specularSamples)
		{
			RGBColor I(0.0, 0.0, 0.0);
			double krefl = 0.1;
			double ka = 1;
			CastedRay cray = CastedRay(ray);

			//cas si la profondeur de la scene superieur
			if (depth > maxDepth) {
				return I;
			}
			
			//verification intersection
			optim(cray,"BVH");


			//Si intersection calcule selon le modele sinon background_color (noir) par defaut
			if (cray.validIntersectionFound()) {
				RGBColor ie=cray.intersectionFound().triangle()->material()->getEmissive();
				RGBColor ia = cray.intersectionFound().triangle()->material()->getAmbient();

				//On ne prend pas en compte ia dans les calculs car elle fausse le r�sultat pour (au moins) sombrero et robot
				I = ie + phongDirect(cray) + reflection(cray, depth + 1, maxDepth, diffuseSamples, specularSamples, krefl);//+ sendRay(r_refraction, depth + 1, maxDepth, diffuseSamples, specularSamples) * krefr;
				//texture
				RGBColor stexture = cray.intersectionFound().triangle()->sampleTexture(cray.intersectionFound().uTriangleValue(), cray.intersectionFound().vTriangleValue());
				I = I * stexture;
			}
			return I;
		}


		
		RGBColor pathTracing(Ray const & ray,int depth,int maxDepth, int diffuseSamples, int specularSamples)
		{
			//step 0 : init
			CastedRay cray = CastedRay(ray);
			optim(cray, "BVH");
			
			//step 1 : intersection find
			if (cray.validIntersectionFound()) {
				//Generate uniform random p to bounce the ray or not - russian roulette
				double p = ((double)rand() / (RAND_MAX));
				double absorption = 1 - p;
				RGBColor Le = cray.intersectionFound().triangle()->material()->getEmissive();
				RGBColor stexture = cray.intersectionFound().triangle()->sampleTexture(cray.intersectionFound().uTriangleValue(), cray.intersectionFound().vTriangleValue());

				if (p < absorption) {
					//step 3 - recurssion : Generate a new ray in random direction from intersection
					const Math::Vector3f N = cray.intersectionFound().triangle()->sampleNormal(cray.intersectionFound().uTriangleValue(), cray.intersectionFound().vTriangleValue(), cray.source()); //surface normal
					const Math::Vector3f source = cray.intersectionFound().intersection();																																											
					Math::RandomDirection rdirection = Math::RandomDirection(N.normalized());

					//send multiple ray to reduce noise --> crash
					int nbRay = 1;
					RGBColor rayColorSum(0.0, 0.0, 0.0);
					for (int i = 0; i < nbRay; i++)
					{
						CastedRay randomRay = CastedRay(source, rdirection.generate());
						RGBColor rayColor = pathTracing(randomRay, depth, maxDepth, diffuseSamples, specularSamples) *absorption;
						
						rayColorSum = rayColorSum + rayColor / nbRay;
					}

					return Le + phongDirect(cray)*stexture+rayColorSum;
				}
				else {
					//step 3 - stop recuression 
					return Le + phongDirect(cray)*stexture;
				}
			}
			else {
				return RGBColor(0.0, 0.0, 0.0); //background color
			}
		}

		RGBColor phongDirect(CastedRay const &cray) {
			RGBColor result(0.0, 0.0, 0.0);
			
			//Global Illumination
			if (m_GI_surface) {

				for (LightSource * source : m_lightSampler) {
				
					PointLight light = source->generate();
					if (!phongShadow(cray, light)) {
						//pas dans l'ombre donc on calcule
						result = result + (phongDiffuse(cray, light) + phongSpecular(cray, light))*light.color();
						}
					
				}
			}

			//Classic Raytracing
			else {
				//On verifie pour chaque lumiere si celle si eclaire notre point d'intersection
				for (const PointLight & light : m_lights) {
					if (!phongShadow(cray, light)) {
						//pas dans l'ombre donc on calcule
						result = result + (phongDiffuse(cray, light) + phongSpecular(cray, light))*light.color();
					}
				}
			}
			return result;
		}

		RGBColor phongDiffuse(CastedRay const &cray, PointLight const &light) {
			RGBColor i_diffuse(0.0, 0.0, 0.0);

			RGBColor kd = cray.intersectionFound().triangle()->material()->getDiffuse();

			//normal du triangle intersecte
			Math::Vector3f N = cray.intersectionFound().triangle()->sampleNormal(cray.intersectionFound().uTriangleValue(), cray.intersectionFound().vTriangleValue(), cray.source());
			//Math::Vector3f N = cray.intersectionFound().triangle()->normal();
			//Si le produit scalaire entre la normale du triangle et la direction du regard est negatif, on prend l'oppos� de la normale du triangle
			if (N*cray.direction() < 0) N = -N;

			//direction de la l'intersection vers la light
			Math::Vector3f L = cray.intersectionFound().intersection() - light.position();

			i_diffuse = kd * ((double)::std::max(0.0, N.normalized()*L.normalized()));
			//influence par rapport a la distance de la source de lumiere
			i_diffuse = i_diffuse / L.norm();

			return i_diffuse;
		}

		bool phongShadow(CastedRay const &cray, PointLight const &light) {
			//retourne true si dans l'ombre
			bool shadow = false;
			CastedRay cshadow(light.position(), cray.intersectionFound().intersection() - light.position());

			optim(cshadow, "BVH");
			if (cshadow.validIntersectionFound()) {
				shadow = !(cshadow.intersectionFound().triangle()->center() == cray.intersectionFound().triangle()->center());
			}

			return shadow;
		}

		RGBColor phongSpecular(CastedRay const &cray, PointLight const &light) {
			RGBColor i_specular(0.0, 0.0, 0.0);

			RGBColor ks = cray.intersectionFound().triangle()->material()->getSpecular();
			Math::Vector3f V = -cray.direction();
			Math::Vector3f L = cray.intersectionFound().intersection() - light.position();
			Math::Vector3f N = cray.intersectionFound().triangle()->sampleNormal(cray.intersectionFound().uTriangleValue(), cray.intersectionFound().vTriangleValue(), cray.source());
			//Direction de reflexion, avec interpolation
			Math::Vector3f R = cray.intersectionFound().triangle()->reflectionDirection(N.normalized(),L.normalized());

			i_specular = ks * pow(::std::max(V*R, 0.0), cray.intersectionFound().triangle()->material()->getShininess());

			//influence par rapport a la distance de la source de lumiere
			i_specular = i_specular / L.norm();

			return i_specular;
		}

		RGBColor reflection(CastedRay const & cray, int const &depth, int const &maxDepth, int const & diffuseSamples, int const &specularSamples, double const &krefl)
		{
			//Eclairage indirect
			//normal du triangle intersecte
			Math::Vector3f N = cray.intersectionFound().triangle()->sampleNormal(cray.intersectionFound().uTriangleValue(),cray.intersectionFound().vTriangleValue(), cray.source());
			//Verification si on prend la normale dans la bonne direction
			//if (N*cray.direction() < 0) N = -N;

			CastedRay creflection(cray.intersectionFound().intersection(), cray.intersectionFound().triangle()->reflectionDirection(N.normalized(),cray.direction()));

			return cray.intersectionFound().triangle()->material()->getSpecular()*sendRay(creflection, depth + 1, maxDepth, diffuseSamples, specularSamples)*krefl;
		}

		void optim(CastedRay &cray, char* s="") {
			if (s=="BVH") {
				//BVH
				m_bvh->path(cray);
			}
			else {
				optimTemp(cray);
			}
		}

		void optimTemp(CastedRay &cray) {
			double t0 = 0.0;
			double t1 = 100000.0;
			double entry = 0.0;
			double exit = 0.0;

			for (::std::pair<BoundingBox, Geometry> &geo : m_geometries)
			{
				BoundingBox box = geo.first;
				if (!box.isEmpty()) {
					if (box.intersect(cray, t0, t1, entry, exit)) {
						for (const Triangle &t : geo.second.getTriangles()) {
							cray.intersect(&t);
						}
					}
				}
			}
		}

		void buildBVH() {
			m_bvh = new BVH(m_geometries, m_sceneBoundingBox);
		}


		////////////////////////////////////////////////////////////////////////////////////////////////////
		/// \fn	void Scene::compute(int maxDepth)
		///
		/// \brief	Computes a rendering of the current scene, viewed by the camera.
		/// 		
		/// \author	F. Lamarche, Universit� de Rennes 1
		/// \date	04/12/2013
		///
		/// \param	maxDepth	The maximum recursive depth.
		/// \param  subPixelDivision subpixel subdivisions to handle antialiasing
		////////////////////////////////////////////////////////////////////////////////////////////////////
		void compute(int maxDepth, int subPixelDivision = 1, int passPerPixel = 1)
		{
			
			buildBVH();
			// We prepare the light sampler (the sampler only stores triangles with a non null emissive component).
			/*
			for (auto it = m_geometries.begin(), end = m_geometries.end(); it != end; ++it)
			{
				//emissive a voir
				//m_lightSampler.add(it->second);
				m_SAMPLER.add(it->second);
			}
			*/

			// Step on x and y for subpixel sampling
			double step = 1.0f/subPixelDivision ;
			// Table accumulating values computed per pixel (enable rendering of each pass)
			::std::vector<::std::vector<::std::pair<int, RGBColor> > > pixelTable(m_visu->width(), ::std::vector<::std::pair<int, RGBColor> >(m_visu->width(), ::std::make_pair(0, RGBColor()))) ;

			// 1 - Rendering time
			LARGE_INTEGER frequency;        // ticks per second
			LARGE_INTEGER t1, t2;           // ticks
			double elapsedTime;
			// get ticks per second
			QueryPerformanceFrequency(&frequency);
			// start timer
			QueryPerformanceCounter(&t1);
			// Rendering pass number
			m_pass = 0;
			// Rendering

			for(int passPerPixelCounter = 0 ; passPerPixelCounter<passPerPixel ; ++passPerPixelCounter)
			{
				for (double xp = -0.5; xp < 0.5; xp += step)
				{
					for (double yp = -0.5; yp < 0.5; yp += step)
					{
						int newSeed = std::rand();

						::std::cout << "Pass: " << m_pass << "/" << passPerPixel * subPixelDivision * subPixelDivision << ::std::endl;
						++m_pass;
						// Sends primary rays for each pixel (uncomment the pragma to parallelize rendering)
#pragma omp parallel for schedule(dynamic)//, 10)//guided)//dynamic)
						for (int y = 0; y < m_visu->height(); y++)
						{
							for (int x = 0; x < m_visu->width(); x++)
							{
#pragma omp critical (visu)
								m_visu->plot(x, y, RGBColor(1000.0, 0.0, 0.0));
								//Echantillonnage
								if (m_GI_graineUnique) std::srand(newSeed);
								// Ray casting
								RGBColor result;
								if (m_GI_indirect) {
									result = pathTracing(m_camera.getRay(((double)x + xp) / m_visu->width(), ((double)y + yp) / m_visu->height()), 0, maxDepth, m_diffuseSamples, m_specularSamples);
								}
								else {
									result = sendRay(m_camera.getRay(((double)x + xp) / m_visu->width(), ((double)y + yp) / m_visu->height()), 0, maxDepth, m_diffuseSamples, m_specularSamples);
								}
								
								// Accumulation of ray casting result in the associated pixel
								::std::pair<int, RGBColor> & currentPixel = pixelTable[x][y];
								currentPixel.first++;
								currentPixel.second = currentPixel.second + result;
								// Pixel rendering (with simple tone mapping)
#pragma omp critical (visu)
								m_visu->plot(x, y, pixelTable[x][y].second / (double)(pixelTable[x][y].first) * 10);
								// Updates the rendering context (per pixel) - warning per pixel update can be costly...
//#pragma omp critical (visu)
								//m_visu->update();
							}
							// Updates the rendering context (per line)
#pragma omp critical (visu)
							m_visu->update();
						}
						// Updates the rendering context (per pass)
						//m_visu->update();
						// We print time for each pass
						QueryPerformanceCounter(&t2);
						elapsedTime = (double)(t2.QuadPart - t1.QuadPart) / (double)frequency.QuadPart;
						double remainingTime = (elapsedTime / m_pass)*(passPerPixel * subPixelDivision * subPixelDivision - m_pass);
						::std::cout << "time: " << elapsedTime << "s. " <<", remaining time: "<< remainingTime << "s. " <<", total time: "<< elapsedTime + remainingTime << ::std::endl;
					}
				}
			}
			// stop timer
			QueryPerformanceCounter(&t2);
			elapsedTime = (double)(t2.QuadPart - t1.QuadPart) / (double)frequency.QuadPart;
			::std::cout<<"time: "<<elapsedTime<<"s. "<<::std::endl ;
		}
	} ;


}

#endif
