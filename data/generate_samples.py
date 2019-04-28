import datetime
import random
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

to_file = True

max_pressure = 1020.
min_pressure = 980.
noise_mbar = 0.5

pressure_range = max_pressure-min_pressure
center_pressure = (min_pressure+max_pressure)/2.

sample_period_seconds = 300
utc = datetime.datetime.utcnow()
now = utc.replace(tzinfo=datetime.timezone.utc).timestamp()
end = sample_period_seconds * int(now/sample_period_seconds)
span = 6*24*60*60
start = end-span

# for varying series depending on generation time
# offset=0

# for series with fixed shape independantly of generation time
offset = start

#print("Current unix timestamp = %d " % int(now))

def pressure(x):
    weekly = pressure_range/2. * np.cos((x-offset)*np.pi/float(4*24*3600))
    diurnal = 3. * np.sin((x-offset)*2.*np.pi/float(24*3600)
                          ) + noise_mbar*random.random()
    return center_pressure + weekly + diurnal


stamps = np.arange(start, end, sample_period_seconds)

values = np.array([pressure(s) for s in stamps])

if to_file:
    with open('generated.h', 'w') as out:
        out.write("const uint32_t kFirtTimeStamp=%d;\n" % start)
        out.write("const uint32_t kLastTimeStamp=%d;\n" % end)
        out.write("const uint32_t kNumberOfSamples=%d;\n" % len(stamps))
        out.write("int32_t seconds_and_pressure[%d][2] = {\n" % len(stamps))
        index = 0
        for s, p in np.nditer([stamps, values]):
            index = index+1
            out.write("  {%d, %d}" % (int(s), int(p*100)))
            if index < len(stamps):
                out.write(",\n")
            else:
                out.write("\n")
        out.write("};\n")
        out.close()

else:
    indices = np.array([datetime.datetime.fromtimestamp(s) for s in stamps])
    data = pd.Series(values, indices)
    plt.plot(data)
    plt.show()
