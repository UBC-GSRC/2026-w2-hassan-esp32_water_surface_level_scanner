import serial
import WaterSurfaceScanner
import CameraCart
import numpy as np
import time
import numpy as np
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D

def viewer(data):
    fig = plt.figure()
    ax = fig.add_subplot(111, projection="3d")

    scatter = ax.scatter(data[:,2], data[:,3], data[:,4], c=data[:,1], cmap='hsv', marker="o")

    # Add color bar to show mapping
    cbar = plt.colorbar(scatter, ax=ax, shrink=0.6)
    cbar.set_label("Label")

    ax.set_xlabel("X")
    ax.set_ylabel("Y")
    ax.set_zlabel("Z")
    ax.set_title("3D Scatter Plot of XYZ Data")

    plt.show()

def main():
    WSS = WaterSurfaceScanner.WaterSurfaceScanner()
    CC = CameraCart.CameraCart()
    WSS.start_recording() # This starts alternate threads so it can still record while being blocked in the main script
    # CC.jog_absolute(4000) # This is blocking 
    CC.jog_relative(-2300)
    # CC.com.move(CC.cartAxle, -400, 'r')
    WSS.stop_recording()
    WSS.define_transforms()
    WSS.add_transforms()
    data = WSS.get_water_surface_data()

    np.savetxt(f"surface_data_test_{round(time.time())}.csv", data, delimiter=",")
    viewer(data)



if __name__ == "__main__":
    main()
