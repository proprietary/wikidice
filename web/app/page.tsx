'use client';

import Image from "next/image";
import styles from "./page.module.css";
import { Autocomplete } from "./autocomplete";
import { useState } from "react";
import { lookup } from '../api';
import React from 'react';

export default function Home() {
  const [query, setQuery] = useState("");
  const [wikipediaUrl, setWikipediaUrl] = useState("");
  const [derivation, setDerivation] = useState<Array<string>>([]);
  const handleSubmit = async (query: string) => {
    setQuery(query);
    console.log(query);
    const result = await lookup(query);
    if (result) {
      const [pageId, derivation] = result;
      setWikipediaUrl(`https://en.wikipedia.org/?curid=${pageId}`);
      setDerivation(derivation);
    }
  };
  return (
    <main>
      <div>
        <Autocomplete placeholder="Physics" buttonText="Submit" onSubmit={handleSubmit}  />
      </div>
      <div>
        {wikipediaUrl.length > 0 && (
          <div>
            <div>
              <p>Found via:</p>
              <div style={{ display: 'flex', alignItems: 'center' }}>
                {derivation.map((cat, i, arr) => (
                  <React.Fragment key={i}>
                    <span>
                      <span onClick={() => { setQuery(cat); handleSubmit(cat); }} style={{cursor: 'pointer', color: 'blue', marginRight: '0.5rem'}}>{cat}</span>
                      <a href={`https://en.wikiepdia.org/wiki/Category:${cat.replace(' ', '_')}`}>
                        <svg xmlns="http://www.w3.org/2000/svg" stroke="currentColor" strokeWidth={8} fill="none" width="12" height="12" viewBox="0 0 100 100">
                          <path d="m43,35H5v60h60V57M45,5v10l10,10-30,30 20,20 30-30 10,10h10V5z" />
                        </svg>
                      </a>
                    </span>
                    {i < arr.length - 1 && (
                      <div style={{width: '20px', height: 'auto', margin: '0 10px'}}>
                        <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" strokeWidth={1.5} stroke="currentColor">
                          <path strokeLinecap="round" strokeLinejoin="round" d="M17.25 8.25 21 12m0 0-3.75 3.75M21 12H3" />
                        </svg>
                      </div>
                    )}
                  </React.Fragment>
                ))}
              </div>
            </div>
            <iframe src={wikipediaUrl} style={{width: '100vw', height: '80vh'}} />
          </div>
        )}
      </div>
    </main>
  );
}
