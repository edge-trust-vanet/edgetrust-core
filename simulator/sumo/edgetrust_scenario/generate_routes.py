import os

def write_routes(filename, end_time, probability):
    routes = [
        "-E11 E10",  # West -> East
        "-E10 E11",  # East -> West
        "-E12 E13",  # South -> North
        "-E13 E12",  # North -> South
        "-E11 E13",  # West -> North
        "-E13 E10",  # North -> East
        "-E10 E12",  # East -> South
        "-E12 E11"   # South -> West
    ]
    
    with open(filename, 'w') as f:
        f.write('<?xml version="1.0" encoding="UTF-8"?>\n\n')
        f.write('<routes>\n')
        f.write('    <vType id="car" accel="2.6" decel="4.5" sigma="0.5" length="5" minGap="2.5" maxSpeed="13.9" color="0,0,255"/>\n\n')
        
        # Define routes
        for i, r in enumerate(routes):
            f.write(f'    <route id="route{i}" edges="{r}"/>\n')
        
        f.write('\n')
        
        # Define flows
        for i, r in enumerate(routes):
            f.write(f'    <flow id="flow{i}" type="car" begin="0" end="{end_time}" probability="{probability}" route="route{i}"/>\n')
            
        f.write('</routes>\n')

# Low density (e.g., 5% chance per second per route = very sparse)
write_routes('routes_low.rou.xml', 100, 0.05)

# Medium density (e.g., 20% chance per second per route = standard traffic)
write_routes('routes_medium.rou.xml', 100, 0.20)

# High density (e.g., 60% chance per second per route = heavy congestion)
write_routes('routes_high.rou.xml', 100, 0.60)

print("Successfully generated routes_low.rou.xml, routes_medium.rou.xml, and routes_high.rou.xml!")
