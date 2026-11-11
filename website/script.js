// --- 1. Animated Starfield Background ---
const canvas = document.getElementById('starfield');
if (canvas) {
    const ctx = canvas.getContext('2d');
    let stars = [];

    function initStars() {
        canvas.width = window.innerWidth;
        canvas.height = window.innerHeight;
        stars = [];
        for (let i = 0; i < 400; i++) {
            stars.push({
                x: Math.random() * canvas.width,
                y: Math.random() * canvas.height,
                radius: Math.random() * 1.5,
                speed: Math.random() * 0.5 + 0.1
            });
        }
    }

    function animateStars() {
        ctx.clearRect(0, 0, canvas.width, canvas.height);
        ctx.fillStyle = '#ffffff';
        stars.forEach(star => {
            ctx.beginPath();
            ctx.arc(star.x, star.y, star.radius, 0, Math.PI * 2);
            ctx.fill();
            star.y -= star.speed;
            if (star.y < 0) {
                star.y = canvas.height;
                star.x = Math.random() * canvas.width;
            }
        });
        requestAnimationFrame(animateStars);
    }

    window.addEventListener('resize', initStars);
    initStars();
    animateStars();
}

// --- 2. Planet Data (Word File Description Formatted) ---
const planetsData = [
    {
        name: "Sun (Sol)",
        type: "G-type main-sequence star (G2V)",
        image: "./images/sun.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">The Sun is a G-type main-sequence star (G2V), commonly called a yellow dwarf. It contains <strong>99.86%</strong> of all mass in the Solar System, driving space weather, planetary climates, and orbital motion through its powerful gravitational field.</p>
            
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Mass:</strong> 1.989 × 10³⁰ kg (333,000 Earths) &nbsp;|&nbsp; <strong>Radius:</strong> 696,340 km (109 Earth radii) &nbsp;|&nbsp; <strong>Gravity:</strong> 274 m/s² (27.9 g)</p>
            <p class="info-text"><strong>Rotation Period:</strong> 25 days at the equator to 35 days near the poles (Differential Rotation).</p>

            <h3>Internal & Atmospheric Layers</h3>
            <p class="info-text"><strong>Internal:</strong> Nuclear Fusion Core (Hydrogen to Helium via p-p chain) → Radiative Zone → Convective Zone.</p>
            <p class="info-text"><strong>Atmosphere:</strong> Photosphere → Chromosphere → Transition Region → Corona. <br> 
            <strong>Temperature:</strong> Surface (Photosphere) 5,500°C; Core 15.7 Million°C; Corona 1 - 3 Million°C.</p>

            <h3>Weather & Acoustic Propagation</h3>
            <p class="info-text"><strong>Weather:</strong> Solar wind (proton/electron plasma flux), coronal mass ejections (CMEs), solar flares, and geomagnetic storms.</p>
            <p class="info-text"><strong>Sounds:</strong> Inaudible in empty space vacuum, but helioseismology records acoustic pressure wave loops vibrating inside the Sun's plasma core.</p>
        `
    },
    {
        name: "Mercury",
        type: "Terrestrial Planet",
        image: "./images/mercury.jpg",
        description: `
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Size:</strong> 2,439.7 km radius (0.38 Earths) &nbsp;|&nbsp; <strong>Mass:</strong> 3.30 × 10²³ kg (0.055 Earths) &nbsp;|&nbsp; <strong>Gravity:</strong> 3.70 m/s² (0.38 g)</p>
            <p class="info-text"><strong>Rotation:</strong> 58.6 Earth days (3:2 spin-orbit resonance) &nbsp;|&nbsp; <strong>Orbital Period:</strong> 88 Earth days &nbsp;|&nbsp; <strong>Temperature:</strong> Extreme diurnal variation: Day +430℃ to Night -180℃.</p>

            <h3>Atmosphere, Weather & Geology</h3>
            <p class="info-text"><strong>Atmosphere:</strong> Surface-bound exosphere: 42% Oxygen (O₂), 29% Sodium (Na), 22% Hydrogen (H₂), 6% Helium (He), 0.5% Potassium.</p>
            <p class="info-text"><strong>Weather:</strong> No atmospheric weather, rain, or wind due to negligible atmosphere.</p>
            <p class="info-text"><strong>Surface:</strong> Silicate regolith, heavily cratered surface covered in fine dust, graphite, and volcanic basaltic plains.</p>

            <h3>Water, Internal Structure & Acoustics</h3>
            <p class="info-text"><strong>Water:</strong> No liquid water. Substantial water ice permanently shadowed inside polar impact craters.</p>
            <p class="info-text"><strong>Internal Layers:</strong> Massive metallic iron core (~85% of planetary radius) → Thin silicate mantle → Brittle rocky crust.</p>
            <p class="info-text"><strong>Sounds:</strong> Vacuum silence. Magnetospheric plasma waves detected by radio telemetry.</p>
            <p class="info-text"><strong>Moons:</strong> No natural satellites.</p>
        `
    },
    {
        name: "Venus",
        type: "Terrestrial Planet (Greenhouse Hellscape)",
        image: "./images/venus.jpg",
        description: `
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Size:</strong> 6,051.8 km radius (0.95 Earths) &nbsp;|&nbsp; <strong>Mass:</strong> 4.87 × 10²⁴ kg (0.815 Earths) &nbsp;|&nbsp; <strong>Gravity:</strong> 8.87 m/s² (0.90 g)</p>
            <p class="info-text"><strong>Rotation:</strong> 243 Earth days (Retrograde - spins clockwise, sun rises in West) &nbsp;|&nbsp; <strong>Orbital Period:</strong> 225 Earth days &nbsp;|&nbsp; <strong>Temperature:</strong> Uniform ~465°C globally due to extreme runaway greenhouse effect.</p>

            <h3>Atmosphere & Weather</h3>
            <p class="info-text"><strong>Atmosphere:</strong> 96.5% Carbon Dioxide (CO₂), 3.5% Nitrogen (N₂), trace Sulfur Dioxide (SO₂). Surface pressure: 92 bar (equivalent to 900m under ocean).</p>
            <p class="info-text"><strong>Weather:</strong> Hurricane-force upper atmospheric winds (~360 km/h). Sulfuric acid (H₂SO₄) rain that evaporates mid-air (Virga) before reaching the ground.</p>

            <h3>Geology, Water & Structure</h3>
            <p class="info-text"><strong>Surface:</strong> Basaltic volcanic rock, vast lava plains, shield volcanoes, and highly fractured tessera terrain.</p>
            <p class="info-text"><strong>Water:</strong> Completely dry surface. Minimal atmospheric water vapor (~20 ppm).</p>
            <p class="info-text"><strong>Internal Layers:</strong> Solid/liquid iron-nickel core → Rocky silicate mantle → Thick volcanic silicate crust.</p>
            <p class="info-text"><strong>Sounds:</strong> Heavy, muffled acoustic transmission through hyper-dense carbon dioxide atmosphere.</p>
            <p class="info-text"><strong>Moons:</strong> No natural satellites.</p>
        `
    },
    {
        name: "Earth",
        type: "Terrestrial Planet (Habitable World)",
        image: "./images/earth.jpg",
        description: `
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Size:</strong> 6,371.0 km radius &nbsp;|&nbsp; <strong>Mass:</strong> 5.97 × 10²⁴ kg &nbsp;|&nbsp; <strong>Gravity:</strong> 9.807 m/s² (1.0 g)</p>
            <p class="info-text"><strong>Rotation:</strong> 23 hours, 56 minutes, 4 seconds (1 Sidereal Day) &nbsp;|&nbsp; <strong>Orbital Period:</strong> 365.25 Earth days &nbsp;|&nbsp; <strong>Temperature:</strong> Global average -15°C (Extremes: -89.2°C to +56.7°C).</p>

            <h3>Atmosphere & Weather</h3>
            <p class="info-text"><strong>Atmosphere:</strong> 78.08% Nitrogen (N₂), 20.95% Oxygen (O₂), 0.93% Argon (Ar), 0.04% Carbon Dioxide (CO₂). Layers: Troposphere → Stratosphere → Mesosphere → Thermosphere → Exosphere.</p>
            <p class="info-text"><strong>Weather:</strong> Dynamic hydrologic system: water-ice clouds, rain, snow, hurricanes, tornadoes, lightning, and atmospheric jet streams.</p>

            <h3>Geology, Water & Structure</h3>
            <p class="info-text"><strong>Surface:</strong> Rich organic mineral soil (humus, clay, silt, sand), active plate tectonics, oceanic and continental crusts.</p>
            <p class="info-text"><strong>Water:</strong> Abundant liquid water oceans covering 70.8% of surface area, ice caps, rivers, and lakes.</p>
            <p class="info-text"><strong>Internal Layers:</strong> Solid inner core (Fe-Ni) → Outer liquid core (generates magnetosphere) → Viscous silicate mantle → Lithospheric crust.</p>
            
            <h3>Acoustics & Moons</h3>
            <p class="info-text"><strong>Sounds:</strong> Rich acoustic environment supported by standard atmospheric pressure.</p>
            <p class="info-text"><strong>Moons:</strong> 1 Natural Satellite: Moon (Luna) → Radius 1,737 km, gravity 1.62 m/s², covered in anorthositic regolith.</p>
        `
    },
    {
        name: "Mars",
        type: "Terrestrial Planet (Red Planet)",
        image: "./images/mars.jpg",
        description: `
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Size:</strong> 3,389.5 km radius (0.53 Earths) &nbsp;|&nbsp; <strong>Mass:</strong> 6.42 × 10²³ kg (0.107 Earths) &nbsp;|&nbsp; <strong>Gravity:</strong> 3.72 m/s² (0.38 g)</p>
            <p class="info-text"><strong>Rotation:</strong> 24 hours, 37 minutes, 22 seconds (1 Martian Sol) &nbsp;|&nbsp; <strong>Orbital Period:</strong> 687 Earth days (1.88 Earth years) &nbsp;|&nbsp; <strong>Temperature:</strong> Average -60°C (Ranges from +20°C at equator to -153°C at poles).</p>

            <h3>Atmosphere & Weather</h3>
            <p class="info-text"><strong>Atmosphere:</strong> 95.32% Carbon Dioxide, 2.6% Nitrogen, 1.9% Argon, 0.16% Oxygen. Surface pressure: 0.006 bar (0.6% of Earth's).</p>
            <p class="info-text"><strong>Weather:</strong> Planet-encircling dust storms, dust devils, dry-ice (CO₂) snow fall at high latitudes, low-altitude water-ice clouds.</p>

            <h3>Geology, Water & Structure</h3>
            <p class="info-text"><strong>Surface:</strong> Iron-oxide rich regolith (giving reddish tint), basaltic soil containing perchlorates and silicates.</p>
            <p class="info-text"><strong>Water:</strong> Polar ice caps (water ice + dry ice frost) and massive subsurface water-ice reservoirs.</p>
            <p class="info-text"><strong>Internal Layers:</strong> Dense iron-nickel-sulfur core → Silicate mantle → Solid crust without active plate tectonics.</p>

            <h3>Acoustics & Moons</h3>
            <p class="info-text"><strong>Sounds:</strong> Quiet ambient acoustic propagation, high-frequency sound attenuation due to thin CO₂ atmosphere (recorded by Perseverance rover).</p>
            <p class="info-text"><strong>Moons:</strong> 2 Natural Satellites: 1. Phobos (22.2 km across, rapid orbit), 2. Deimos (12.6 km across, irregular moon).</p>
        `
    },
    {
        name: "Jupiter",
        type: "Gas Giant (Jovian Planet)",
        image: "./images/jupiter.jpg",
        description: `
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Size:</strong> 69,911 km radius (11.2 Earths) &nbsp;|&nbsp; <strong>Mass:</strong> 1.898 × 10²⁷ kg (317.8 Earths, 2.5x all planets combined) &nbsp;|&nbsp; <strong>Gravity:</strong> 24.79 m/s² (2.53 g)</p>
            <p class="info-text"><strong>Rotation:</strong> 9 hours, 56 minutes (Fastest rotation in Solar System) &nbsp;|&nbsp; <strong>Orbital Period:</strong> 11.86 Earth years &nbsp;|&nbsp; <strong>Temperature:</strong> Cloud tops -110°C; Deep core -24,000°C.</p>

            <h3>Atmosphere & Weather</h3>
            <p class="info-text"><strong>Atmosphere:</strong> 89.8% Hydrogen (H₂), 10.2% Helium (He), trace Methane (CH₄), Ammonia (NH₃), Water vapor.</p>
            <p class="info-text"><strong>Weather:</strong> Violent cloud belts, powerful lightning strikes (1,000x Earth strength), and the Great Red Spot (anticyclonic storm running >350 years).</p>

            <h3>Geology, Water & Structure</h3>
            <p class="info-text"><strong>Surface:</strong> No solid surface. Smooth transition from gas phase to compressed fluid and liquid metallic state.</p>
            <p class="info-text"><strong>Water:</strong> Abundant water vapor clouds present beneath upper ammonia clouds.</p>
            <p class="info-text"><strong>Internal Layers:</strong> Gaseous atmosphere → Liquid molecular hydrogen → Liquid metallic hydrogen mantle → Dense rock/ice super-core.</p>

            <h3>Acoustics & Moons</h3>
            <p class="info-text"><strong>Sounds:</strong> Intense magnetospheric whistlers and electromagnetic plasma waves captured by Juno spacecraft.</p>
            <p class="info-text"><strong>Moons:</strong> 95 Confirmed Moons. Major Galilean Moons: <br>1. <strong>Ganymede:</strong> Largest moon, has intrinsic magnetic field. <br>2. <strong>Io:</strong> Most volcanically active body (sulfur volcanism). <br>3. <strong>Callisto:</strong> Ancient, intensely cratered icy-rock surface. <br>4. <strong>Europa:</strong> Global liquid water ocean beneath ice crust (prime target for astrobiology).</p>
        `
    },
    {
        name: "Saturn",
        type: "Gas Giant (Ringed Planet)",
        image: "./images/saturn.jpg",
        description: `
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Size:</strong> 58,232 km radius (9.4 Earths) &nbsp;|&nbsp; <strong>Mass:</strong> 5.68 × 10²⁶ kg (95.2 Earths) &nbsp;|&nbsp; <strong>Gravity:</strong> 10.44 m/s² (1.07 g)</p>
            <p class="info-text"><strong>Rotation:</strong> 10 hours, 33 minutes &nbsp;|&nbsp; <strong>Orbital Period:</strong> 29.45 Earth years &nbsp;|&nbsp; <strong>Temperature:</strong> Cloud tops -140°C.</p>

            <h3>Atmosphere & Weather</h3>
            <p class="info-text"><strong>Atmosphere:</strong> 96.3% Hydrogen, 3.25% Helium, trace Methane, Ammonia.</p>
            <p class="info-text"><strong>Weather:</strong> Equatorial winds up to 1,800 km/h, persistent Hexagonal jet-stream storm at the North Pole.</p>

            <h3>Structure, Water & Ring System</h3>
            <p class="info-text"><strong>Surface:</strong> No solid surface; high-pressure fluid layer transition.</p>
            <p class="info-text"><strong>Water:</strong> Trace amounts in atmosphere and massive ring ice system.</p>
            <p class="info-text"><strong>Internal Layers:</strong> Outer molecular hydrogen → Liquid metallic hydrogen → Dense rock/ice core.</p>
            <p class="info-text"><strong>Rings:</strong> Made of billions of ice asteroids/particles (99% pure water ice, ranging from micrometer dust to house-sized boulders).</p>

            <h3>Acoustics & Moons</h3>
            <p class="info-text"><strong>Sounds:</strong> Saturnian Kilometric Radiation (SKR) radio wave emissions converted into audio frequencies.</p>
            <p class="info-text"><strong>Moons:</strong> 146 Confirmed Moons. Key Moons: <br>1. <strong>Titan:</strong> 2nd largest moon; thick nitrogen atmosphere, liquid methane/ethane lakes. <br>2. <strong>Enceladus:</strong> Ocean moon shooting active water-ice/organic geysers. <br>3. <strong>Rhea:</strong> Heavily cratered icy moon with thin oxygen/CO₂ atmosphere trace.</p>
        `
    },
    {
        name: "Uranus",
        type: "Ice Giant",
        image: "./images/uranus.jpg",
        description: `
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Size:</strong> 25,362 km radius (4.0 Earths) &nbsp;|&nbsp; <strong>Mass:</strong> 8.68 × 10²⁵ kg (14.5 Earths) &nbsp;|&nbsp; <strong>Gravity:</strong> 8.69 m/s² (0.89 g)</p>
            <p class="info-text"><strong>Rotation:</strong> 17 hours, 14 minutes (Retrograde tilt of 97.7° - rotates on its side) &nbsp;|&nbsp; <strong>Orbital Period:</strong> 84.0 Earth years &nbsp;|&nbsp; <strong>Temperature:</strong> -224°C (Lowest recorded).</p>

            <h3>Atmosphere, Weather & Structure</h3>
            <p class="info-text"><strong>Atmosphere:</strong> 82.5% Hydrogen, 15.2% Helium, 2.3% Methane (absorbs red light, giving cyan color).</p>
            <p class="info-text"><strong>Weather:</strong> Extreme seasonal variations due to axial tilt, atmospheric cloud bands, high-altitude diamond precipitation ('diamond rain').</p>
            <p class="info-text"><strong>Surface & Water:</strong> No solid surface; continuous mantle of supercritical icy slush. Extensive superheated water, ammonia, and methane ice ocean mantle.</p>
            <p class="info-text"><strong>Internal Layers:</strong> Small rocky silicate/iron core → Supercritical icy ocean mantle → Hydrogen/Helium atmosphere.</p>

            <h3>Acoustics & Moons</h3>
            <p class="info-text"><strong>Sounds:</strong> Whispering planetary magnetospheric emissions.</p>
            <p class="info-text"><strong>Moons:</strong> 28 Confirmed Moons. Major Moons: <br>1. <strong>Oberon:</strong> Outermost major moon, heavily cratered icy surface. <br>2. <strong>Titania:</strong> Largest moon of Uranus. <br>3. <strong>Ariel, Umbriel, Miranda:</strong> Features extreme fault canyons.</p>
        `
    },
    {
        name: "Neptune",
        type: "Ice Giant (Windiest Planet)",
        image: "./images/neptune.jpg",
        description: `
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Size:</strong> 24,622 km radius (3.86 Earths) &nbsp;|&nbsp; <strong>Mass:</strong> 1.024 × 10²⁶ kg (17.1 Earths) &nbsp;|&nbsp; <strong>Gravity:</strong> 11.15 m/s² (1.14 g)</p>
            <p class="info-text"><strong>Rotation:</strong> 16 hours, 6 minutes &nbsp;|&nbsp; <strong>Orbital Period:</strong> 164.8 Earth years &nbsp;|&nbsp; <strong>Temperature:</strong> Average cloud top -214°C.</p>

            <h3>Atmosphere & Weather</h3>
            <p class="info-text"><strong>Atmosphere:</strong> 80% Hydrogen, 19% Helium, 1.5% Methane. Deep vivid azure blue hue.</p>
            <p class="info-text"><strong>Weather:</strong> Fastest wind speeds in Solar System (up to 2,100 km/h), Great Dark Spot cyclonic storms, cirrus methane clouds.</p>

            <h3>Structure & Water</h3>
            <p class="info-text"><strong>Surface:</strong> No solid surface; high-pressure icy mantle fluid.</p>
            <p class="info-text"><strong>Water:</strong> Deep ocean layer of compressed water, ammonia, and methane slush.</p>
            <p class="info-text"><strong>Internal Layers:</strong> Silicate-iron rock core → Hot dense water-ammonia-methane icy mantle → Hydrogen-Helium gaseous atmosphere.</p>

            <h3>Acoustics & Moons</h3>
            <p class="info-text"><strong>Sounds:</strong> Low-frequency plasma radio waves.</p>
            <p class="info-text"><strong>Moons:</strong> 16 Confirmed Moons. Primary Moon: <br><strong>Triton:</strong> Retrograde orbit (captured Kuiper Belt Object), cryovolcanism shooting nitrogen gas geysers, thin nitrogen atmosphere.</p>
        `
    },

        // --- Moons & Asteroids Data (Add to planetsData array) ---
    {
        name: "Moon",
        type: "Earth's Natural Satellite",
        image: "./images/moon.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">The Moon is Earth's only natural satellite and the fifth largest moon in the Solar System. It is tidally locked, meaning the same side always faces Earth. Its gravitational pull is the primary driver of Earth's ocean tides.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Radius:</strong> 1,737.4 km &nbsp;|&nbsp; <strong>Distance from Earth:</strong> ~384,400 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> 27.32 days</p>
            <p class="info-text"><strong>Gravity:</strong> 1.62 m/s² (0.166 g) &nbsp;|&nbsp; <strong>Surface:</strong> Anorthositic regolith, heavily cratered with dark maria plains.</p>
        `
    },
    {
        name: "Phobos",
        type: "Moon of Mars",
        image: "./images/phobos.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Phobos is the larger and inner of the two Martian moons. It is one of the darkest objects in the Solar System and is heavily cratered, dominated by the massive Stickney crater which is nearly half its width. Phobos orbits so fast that it rises and sets twice a day on Mars.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Mean Radius:</strong> ~11.2 km (22.2 km across) &nbsp;|&nbsp; <strong>Distance from Mars:</strong> ~9,377 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> ~7.65 hours</p>
            <p class="info-text"><strong>Composition:</strong> Carbon-rich (C-type) rock, similar to asteroids, suggesting it may be a captured asteroid.</p>
        `
    },
    {
        name: "Deimos",
        type: "Moon of Mars",
        image: "./images/deimos.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Deimos is the smaller, outer moon of Mars. Unlike Phobos, it orbits at a safer distance and is slowly moving away from Mars. Its surface is smoother than Phobos due to partial filling of craters by dust.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Mean Radius:</strong> ~6.2 km (12.6 km across) &nbsp;|&nbsp; <strong>Distance from Mars:</strong> ~23,460 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> ~30.3 hours</p>
            <p class="info-text"><strong>Composition:</strong> Carbon-rich rock, likely a captured asteroid like Phobos.</p>
        `
    },
    {
        name: "Ganymede",
        type: "Moon of Jupiter (Galilean)",
        image: "./images/ganymede.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Ganymede is the largest moon in the Solar System, larger than the planet Mercury. It is the only moon known to have its own magnetic field. Its surface is a mix of ancient dark cratered terrain and younger, brighter grooved terrain.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Radius:</strong> 2,634.1 km &nbsp;|&nbsp; <strong>Distance from Jupiter:</strong> ~1,070,400 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> 7.15 days</p>
            <p class="info-text"><strong>Structure:</strong> Rocky core, ice/silicate mantle, and an icy crust.</p>
        `
    },
    {
        name: "Io",
        type: "Moon of Jupiter (Galilean)",
        image: "./images/io.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Io is the most volcanically active body in the Solar System. Tidal heating from Jupiter's immense gravity constantly squeezes and flexes Io, driving hundreds of active volcanoes that spew sulfur and sulfur dioxide.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Radius:</strong> 1,821.6 km &nbsp;|&nbsp; <strong>Distance from Jupiter:</strong> ~421,700 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> 1.77 days</p>
            <p class="info-text"><strong>Surface:</strong> Covered in sulfur compounds, giving it a vibrant yellow, orange, and red appearance.</p>
        `
    },
    {
        name: "Callisto",
        type: "Moon of Jupiter (Galilean)",
        image: "./images/callisto.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Callisto is one of the most heavily cratered objects in the Solar System. Its ancient surface has remained largely unchanged for billions of years, indicating very little geological activity.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Radius:</strong> 2,410.3 km &nbsp;|&nbsp; <strong>Distance from Jupiter:</strong> ~1,882,700 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> 16.69 days</p>
            <p class="info-text"><strong>Structure:</strong> Undifferentiated mixture of rock and ice, unlike the fully differentiated Ganymede.</p>
        `
    },
    {
        name: "Europa",
        type: "Moon of Jupiter (Galilean)",
        image: "./images/europa.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Europa is one of the most promising places to search for extraterrestrial life. Beneath its smooth, icy shell lies a global saltwater ocean that may contain more water than all of Earth's oceans combined.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Radius:</strong> 1,560.8 km &nbsp;|&nbsp; <strong>Distance from Jupiter:</strong> ~671,100 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> 3.55 days</p>
            <p class="info-text"><strong>Surface:</strong> Young, smooth ice with characteristic reddish-brown cracks (lineae).</p>
        `
    },
    {
        name: "Rhea",
        type: "Moon of Saturn",
        image: "./images/rhea.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Rhea is Saturn's second-largest moon. It is an icy body with a heavily cratered surface and is one of the few moons known to have a tenuous atmosphere containing oxygen and carbon dioxide.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Radius:</strong> 763.8 km &nbsp;|&nbsp; <strong>Distance from Saturn:</strong> ~527,040 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> 4.52 days</p>
        `
    },
    {
        name: "Enceladus",
        type: "Moon of Saturn",
        image: "./images/enceladus.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Enceladus is a small, icy moon that has captured the attention of scientists. It actively vents water vapor and ice particles from a subsurface ocean through cracks near its south pole, known as "tiger stripes." These geysers feed Saturn's E-ring.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Radius:</strong> 252.1 km &nbsp;|&nbsp; <strong>Distance from Saturn:</strong> ~238,000 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> 1.37 days</p>
            <p class="info-text"><strong>Key Feature:</strong> Active cryovolcanism and a global subsurface ocean.</p>
        `
    },
    {
        name: "Titan",
        type: "Moon of Saturn",
        image: "./images/titan.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Titan is Saturn's largest moon and the only moon in the Solar System with a dense atmosphere. It has a thick nitrogen atmosphere with clouds and rain, and lakes and rivers of liquid methane and ethane on its surface.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Radius:</strong> 2,574.7 km &nbsp;|&nbsp; <strong>Distance from Saturn:</strong> ~1,221,870 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> 15.95 days</p>
            <p class="info-text"><strong>Atmosphere:</strong> ~95% Nitrogen, ~5% Methane.</p>
        `
    },
    {
        name: "Oberon",
        type: "Moon of Uranus",
        image: "./images/oberon.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Oberon is the outermost of Uranus's major moons. It is an icy world with a dark, heavily cratered surface that is one of the oldest in the Uranian system.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Radius:</strong> 761.4 km &nbsp;|&nbsp; <strong>Distance from Uranus:</strong> ~583,520 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> 13.46 days</p>
        `
    },
    {
        name: "Triton",
        type: "Moon of Neptune",
        image: "./images/triton.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Triton is Neptune's largest moon and is unique because it orbits backwards (retrograde) compared to Neptune's rotation. This suggests it is a captured Kuiper Belt Object. It has active geysers that shoot nitrogen gas into space.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Radius:</strong> 1,353.4 km &nbsp;|&nbsp; <strong>Distance from Neptune:</strong> ~354,759 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> 5.88 days (retrograde)</p>
        `
    },
    {
        name: "Hygiea",
        type: "Asteroid (Main Belt)",
        image: "./images/hygiea.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Hygiea is the fourth-largest asteroid in the Solar System. It is a C-type (carbonaceous) asteroid and is considered a possible dwarf planet due to its nearly spherical shape.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Diameter:</strong> ~434 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> ~5.5 Earth years</p>
        `
    },
    {
        name: "Pallas",
        type: "Asteroid (Main Belt)",
        image: "./images/pallas.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Pallas is the third-largest asteroid in the Solar System. It is notable for its highly inclined orbit and its unusually tilted rotation axis, similar to Uranus.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Diameter:</strong> ~538 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> ~4.62 Earth years</p>
        `
    },
    {
        name: "Vesta",
        type: "Asteroid (Main Belt)",
        image: "./images/vesta.jpg",
        description: `
            <h3>Overview</h3>
            <p class="info-text">Vesta is the second-largest asteroid and the brightest as seen from Earth. It has a differentiated interior with a crust, mantle, and core, similar to a planet. A massive impact near its south pole created a huge crater and ejected many meteorites that have landed on Earth.</p>
            <h3>Physical Data</h3>
            <p class="info-text"><strong>Diameter:</strong> ~576 km &nbsp;|&nbsp; <strong>Orbital Period:</strong> ~3.63 Earth years</p>
        `
    }


];

// --- 3. Logic for Main Page (Card Generation) ---
const planetContainer = document.getElementById('planet-container');
if (planetContainer) {
    planetsData.forEach(planet => {
        const card = document.createElement('div');
        card.className = 'col-md-4 col-lg-3';
        card.innerHTML = `
            <div class="planet-card" onclick="window.location.href='planet-details.html?name=${encodeURIComponent(planet.name)}'">
                <img src="${planet.image}" alt="${planet.name}">
                <h4>${planet.name}</h4>
                <p class="planet-type-text">${planet.type}</p>
            </div>
        `;
        planetContainer.appendChild(card);
    });
}

// --- 4. Logic for Planet Details Page ---
const detailsContainer = document.getElementById('details-container');
if (detailsContainer) {
    const urlParams = new URLSearchParams(window.location.search);
    const planetName = urlParams.get('name');
    
    if (planetName) {
        const planet = planetsData.find(p => p.name === planetName);
        if (planet) {
            let detailsHTML = `
                <div class="hero-image-container">
                    <img src="${planet.image}" alt="${planet.name}">
                </div>
                <div class="planet-title">
                    <h1>${planet.name}</h1>
                    <p>${planet.type}</p>
                </div>
                <div class="info-box">
                    ${planet.description}
                </div>
            `;
            detailsContainer.innerHTML = detailsHTML;
        }
    }
}

// --- 5. Modal Function ---
function openSimulation() {
    const modalElement = document.getElementById('simulationModal');
    if(modalElement) {
        const modal = new bootstrap.Modal(modalElement);
        modal.show();
    }
}