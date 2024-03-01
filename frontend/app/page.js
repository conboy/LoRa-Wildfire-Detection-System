"use client"

import Navbar from "@/components/Navbar";
import dynamic from 'next/dynamic'


export default function Home() {
  const Map = dynamic(
    () => import('@/components/Map'), // replace '@components/map' with your component's location
    { ssr: false } // This line is important. It's what prevents server-side render
  )

  return (
    <main className="flex flex-col h-screen">
      <div className="flex-shrink-0">
        <Navbar />
      </div>
      <div className="flex-grow">
        <Map />
      </div>
  </main>
  );
}
