import re
import matplotlib.pyplot as plt
import math
import sys



if __name__ == "__main__":

    rf = open(sys.argv[1])
    rf_str = rf.read()
    rf.close()
    rf_str = re.sub('\s+', ' ', rf_str).strip()
    rf_array = rf_str.split(' ')
    rf_data = [int(x) for x in rf_array]


    plt.style.use('grayscale')
    plt.plot(range(len(rf_data)), rf_data, ',', label="RAW")
    plt.xlabel('Sample index')
    plt.ylabel('ADC output')
    plt.title('ADC raw data')
    plt.legend()
    plt.show()

    f = open(sys.argv[2])
    data_str = f.read()
    f.close()

    data_str = re.sub('\s+', ' ', data_str).strip()
    data_array = data_str.split(' ')
    
    x = []
    re = []
    im = []
    pwr = []

    for e in data_array:
        tmp = e.split(':')
        r = float(tmp[1])
        i = float(tmp[2])
        x += [int(tmp[0])]
        re += [r]
        im += [i]
        
        p = (r**2 + i**2)**(1.0/2.0)
        pwr += [p]

    print(len(x)/2)

    plt.style.use('grayscale')
    plt.plot(x, pwr, '.', label="raw")
    #plt.stem(x, pwr, markerfmt=' ')
    plt.xlabel('Frequency index')
    plt.ylabel('Power')
    plt.title('Power spectrum with DC')
    plt.show()

    plt.style.use('grayscale')
    plt.plot(x[1:], pwr[1:], '.', label="raw")
    #plt.stem(x, pwr, markerfmt=' ')
    plt.xlabel('Frequency index')
    plt.ylabel('Power')
    plt.title('Power spectrum')
    plt.show()

    px = [math.log(i,2) for i in x[1:int(len(x)/2)+1]]
    py = [math.log10(i) for i in pwr[1:int(len(pwr)/2)]]
    
    plt.style.use('grayscale')
    #plt.plot(x[:], pwr[:], label="raw")
    plt.plot(px, pwr[1:int(len(pwr)/2)+1], '.', label="raw")
    #plt.stem(x, pwr, markerfmt=' ')
    plt.xlabel('Frequency index [log2]')
    plt.ylabel('Power')
    plt.title('Power spectrum')
    plt.show()


    print('DC %d' % pwr[0])
    print('HI_FREQ idx %d' % (len(pwr)/2-1))
    print("P HI_FREQ %d" % pwr[int(len(pwr)/2-1)])
    
    if len(sys.argv) >= 4:
        #compute power index
        signal_spec = float(sys.argv[3])
        m = int((len(pwr)/2)-1)
        m0 = int(m * signal_spec);

        print("-----------------------")
        print('signal spec %f' % signal_spec)
        print("Highest FREQ idx: %d" % m)
        print("Signal FREQ idx: %d" % m0)

        del_m0 = 0
        del_m = 0

        #for i in range(0, m0+1):
        for i in range(0, m0+1):
            del_m0 += pwr[i]

        for i in range(0, m+1):
            del_m += pwr[i]

        n = del_m0/del_m

        print("n idx: %f" % n)





