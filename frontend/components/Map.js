import { MapContainer, Marker, Popup, TileLayer, Circle } from 'react-leaflet'
import { useState } from 'react'
import 'leaflet/dist/leaflet.css'

const Map = () => {

  const [nodes, setNodes] = useState([]);

  return (
    <MapContainer center={[44.230687, -76.481323]} zoom={13} scrollWheelZoom={true} style={{height: "100%", width: "100%"}}>
      <TileLayer
        attribution='&copy; <a href="http://osm.org/copyright">OpenStreetMap</a> contributors'
        url="https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png"
      />
      <Marker position={[44.230687, -76.481323]}>
        <Popup>
          A pretty CSS3 popup. <br /> Easily customizable.
        </Popup>
      </Marker>
      <Circle center={[44.230687, -76.481323]} radius={200} />
    </MapContainer>
  )
}

export default Map